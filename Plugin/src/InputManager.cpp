#include "InputManager.hpp"

LRESULT CALLBACK InputManager::_Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        if (HWND hwnd = GetForegroundWindow()) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == DllState::currentProcessId) {
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                    auto vkCode = ((KBDLLHOOKSTRUCT*) lParam)->vkCode;

                    for (auto hook : GetSingleton()->_keyDown_callbacks) {
                        hook(vkCode);
                    }
                }
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

InputManager::InputManager() {
    std::thread([this] {
        auto kbHook = SetWindowsHookEx(
            WH_KEYBOARD_LL,
            _Hook_OnKeyboard,
            DllState::hmodule,
            0
        );
        
        if (kbHook == NULL) {
            FATAL("Failed to attach hook with error {}", GetLastError());
        }

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UnhookWindowsHookEx(kbHook);
    }).detach();
}

void InputManager::register_on_keyDown(keyboard_callback callback) {
    _keyDown_callbacks.push_back(callback);
}

void InputManager::free_on_keyDown(keyboard_callback callback) {
    auto pos = std::find(_keyDown_callbacks.begin(), _keyDown_callbacks.end(), callback);
    if (pos != _keyDown_callbacks.end()) {
        _keyDown_callbacks.erase(pos);
    }
}
