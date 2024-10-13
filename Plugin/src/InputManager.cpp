#include <hidusage.h>

#include "InputManager.hpp"

#define EMIT(name, ...)                                         \
    for (auto hook : GetSingleton()->_## name ##_callbacks) {   \
        if (hook(__VA_ARGS__) == Cancel) {                      \
            return 1;                                           \
        }                                                       \
    }                                                           \

LRESULT CALLBACK InputManager::_Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        HWND hwnd = GetForegroundWindow();
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == DllState::processId) {
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
    return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CALLBACK InputManager::_Hook_OnMouse(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        HWND hwnd = GetForegroundWindow();
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == DllState::processId) {

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
                    break;
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
    return CallNextHookEx(NULL, code, wParam, lParam);
}

WNDPROC oldWindowProc;

LRESULT CALLBACK InputManager::_Hook_WindowMessages(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
    if (hwnd == GetForegroundWindow()) {
        switch (msg) {
            case WM_INPUT: {
                uint32_t dwSize = sizeof(RAWINPUT);
                static uint8_t lpb[sizeof(RAWINPUT)];
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    int xPosRelative = raw->data.mouse.lLastX;
                    int yPosRelative = raw->data.mouse.lLastY;

                    EMIT(mouseMove, xPosRelative, yPosRelative);
                }
                break;
            }
        }
    }
    return CallWindowProc(oldWindowProc, hwnd, msg, wParam, lParam);
}

void InputManager::InterceptRegisterRawInputDevices(PCRAWINPUTDEVICE rids, uint32_t numDevices, uint32_t ridSize) {
    auto self = GetSingleton();

    INFO("Intercepted RegisterRawInputDevices call");

    self->_registerDevicesHook->Disable(); // only needs to fire one time

    auto hwnd = rids[0].hwndTarget;
    DllState::window = hwnd;

    self->SetupHooks(hwnd);
    
    RegisterRawInputDevices(rids, numDevices, ridSize);
}

void InputManager::SetupHooks(HWND hwnd) {
    std::thread([this, hwnd] {

        oldWindowProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)_Hook_WindowMessages);
        INFO("Intercepted window messages");

        auto kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, _Hook_OnKeyboard, DllState::hmodule, 0);
        
        if (kbHook == NULL) {
            FATAL("Failed to attach keyboard hook with error {}", GetLastError());
        }

        auto mouseHook = SetWindowsHookEx(WH_MOUSE_LL, _Hook_OnMouse, DllState::hmodule, 0);

        if (mouseHook == NULL) {
            FATAL("Failed to attach mouse hook with error {}", GetLastError());
        }

        MSG msg;
        while (bool getMessageResult = GetMessage(&msg, 0, 0, 0)) {
            if (getMessageResult == -1) {
                ERROR("Encountered messaging error {}", GetLastError());
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        INFO("Message loop ended, detaching hooks...");

        UnhookWindowsHookEx(kbHook);
        UnhookWindowsHookEx(mouseHook);
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWindowProc);
    }).detach();
}

const auto RegisterRawInputDevicesInjectionSite =
    Memory::get_code_address(0x1d0f29e);
    // AsAddress(dku::Hook::search_pattern<
    //     "f3 0f 11 53 5c "
    //     "48 8b 5c 24 60"
    // >()) + 0xa; // ea8674

InputManager::InputManager() {
    _registerDevicesHook = dku::Hook::AddRelHook<6, true>(
        RegisterRawInputDevicesInjectionSite,
        AsAddress(&InterceptRegisterRawInputDevices)
    );
    _registerDevicesHook->Enable();
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
IMPLEMENT_EVENT(mouseMove_callback, mouseMove);
