#include "InputManager.hpp"

LRESULT CALLBACK InputManager::_Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        if (HWND hwnd = GetForegroundWindow()) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == DllState::currentProcessId) {
                auto vkCode = ((KBDLLHOOKSTRUCT*) lParam)->vkCode;
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                    for (auto hook : GetSingleton()->_keyDown_callbacks) {
                        hook(vkCode);
                    }
                }
                else {
                    for (auto hook : GetSingleton()->_keyUp_callbacks) {
                        hook(vkCode);
                    }
                }
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CALLBACK InputManager::_Hook_OnMouse(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        if (HWND hwnd = GetForegroundWindow()) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == DllState::currentProcessId) {

                int vkCode = 0;
                bool isDown = false;
                switch (wParam) {
                    case WM_LBUTTONDOWN:
                        isDown = true;
                    case WM_LBUTTONUP:
                        vkCode = VK_LBUTTON;
                        break;

                    case WM_RBUTTONDOWN:
                        isDown = true;
                    case WM_RBUTTONUP:
                        vkCode = VK_RBUTTON;
                        break;

                    case WM_MBUTTONDOWN:
                        isDown = true;
                    case WM_MBUTTONUP:
                        vkCode = VK_MBUTTON;
                        break;

                    case WM_XBUTTONDOWN:
                        isDown = true;
                    case WM_XBUTTONUP: {
                        auto xbutton = GET_XBUTTON_WPARAM(((MSLLHOOKSTRUCT*) lParam)->mouseData);
                        switch (xbutton) {
                            case XBUTTON1:
                                vkCode = VK_XBUTTON1;
                                break;
                            case XBUTTON2:
                                vkCode = VK_XBUTTON2;
                                break;
                        }
                        break;
                    }
                }

                if (vkCode != 0) {
                    if (isDown) {
                        for (auto hook : GetSingleton()->_keyDown_callbacks) {
                            hook(vkCode);
                        }
                    }
                    else {
                        for (auto hook : GetSingleton()->_keyUp_callbacks) {
                            hook(vkCode);
                        }
                    }
                }

            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

InputManager::InputManager() {
    std::thread([this] {
        auto kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, _Hook_OnKeyboard, DllState::hmodule, 0);
        
        if (kbHook == NULL) {
            FATAL("Failed to attach keyboard hook with error {}", GetLastError());
        }

        auto mouseHook = SetWindowsHookEx(WH_MOUSE_LL, _Hook_OnMouse, DllState::hmodule, 0);

        if (mouseHook == NULL) {
            FATAL("Failed to attach mouse hook with error {}", GetLastError());
        }

        // Based on https://github.com/Ch4nKyy/BG3WASD/blob/main/src/Hooks/InputHook.cpp, but does it even do anything?
        MSG msg;
        while (GetMessage(&msg, 0, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UnhookWindowsHookEx(kbHook);
        UnhookWindowsHookEx(mouseHook);
    }).detach();
}

#define IMPLEMENT_KEYBOARD_EVENT(name) \
    void InputManager::register_on_## name ##(keyboard_callback callback) {                         \
        _## name ##_callbacks.push_back(callback);                                                  \
    }                                                                                               \
                                                                                                    \
    void InputManager::free_on_## name ##(keyboard_callback callback) {                             \
        auto pos = std::find(_## name ##_callbacks.begin(), _## name ##_callbacks.end(), callback); \
        if (pos != _## name ##_callbacks.end()) {                                                   \
            _## name ##_callbacks.erase(pos);                                                       \
        }                                                                                           \
    }

IMPLEMENT_KEYBOARD_EVENT(keyDown);
IMPLEMENT_KEYBOARD_EVENT(keyUp);
