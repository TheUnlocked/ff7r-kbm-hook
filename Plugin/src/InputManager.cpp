#include "InputManager.hpp"

#define EMIT(name, value)                                       \
    for (auto hook : GetSingleton()->_## name ##_callbacks) {   \
        if (hook(value) == Cancel) {                            \
            return 1;                                           \
        }                                                       \
    }                                                           \

LRESULT CALLBACK InputManager::_Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        if (HWND hwnd = GetForegroundWindow()) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == DllState::currentProcessId) {
                auto vkCode = ((KBDLLHOOKSTRUCT*) lParam)->vkCode;

                int genericCode = 0;
                switch (vkCode) {
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                        genericCode = VK_SHIFT;
                        break;
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                        genericCode = VK_CONTROL;
                        break;
                    case VK_LMENU:
                    case VK_RMENU:
                        genericCode = VK_MENU;
                        break;
                }

                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                    if (genericCode) {
                        EMIT(keyDown, genericCode);
                    }
                    EMIT(keyDown, vkCode);
                }
                else {
                    if (genericCode) {
                        EMIT(keyUp, genericCode);
                    }
                    EMIT(keyUp, vkCode);
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
                auto mouseData = ((MSLLHOOKSTRUCT*) lParam)->mouseData;
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
                        auto xbutton = GET_XBUTTON_WPARAM(mouseData);
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

                    case WM_MOUSEWHEEL: {
                        uint16_t wheelDelta = GET_WHEEL_DELTA_WPARAM(mouseData);
                        EMIT(scroll, wheelDelta);
                    }
                }

                if (vkCode != 0) {
                    if (isDown) {
                        EMIT(keyDown, vkCode);
                    }
                    else {
                        EMIT(keyUp, vkCode);
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

#define IMPLEMENT_EVENT(callback_type, name)                                                        \
    void InputManager::register_on_## name ##(callback_type callback) {                             \
        _## name ##_callbacks.push_back(callback);                                                  \
    }                                                                                               \
                                                                                                    \
    void InputManager::free_on_## name ##(callback_type callback) {                                 \
        auto pos = std::find(_## name ##_callbacks.begin(), _## name ##_callbacks.end(), callback); \
        if (pos != _## name ##_callbacks.end()) {                                                   \
            _## name ##_callbacks.erase(pos);                                                       \
        }                                                                                           \
    }

IMPLEMENT_EVENT(keyboard_callback, keyDown);
IMPLEMENT_EVENT(keyboard_callback, keyUp);
IMPLEMENT_EVENT(scroll_callback, scroll);
