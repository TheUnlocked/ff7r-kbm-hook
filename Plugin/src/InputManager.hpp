#pragma once

#define DECLARE_EVENT(callback_type, name)                      \
    public:                                                     \
        void register_on_## name ##(callback_type callback);    \
        void free_on_## name ##(callback_type callback);        \
    private:                                                    \
        std::vector<callback_type> _## name ##_callbacks;

enum event_result : byte {
    Continue,
    Cancel,
};

class InputManager : public dku::Singleton<InputManager> {
    using keyboard_callback = event_result (*)(int vkCode);
    using scroll_callback = event_result (*)(int16_t delta);
    using mouseMove_callback = event_result (*)(int32_t xDelta, int32_t yDelta);

    DECLARE_EVENT(keyboard_callback, keyDown);
    DECLARE_EVENT(keyboard_callback, keyUp);
    DECLARE_EVENT(scroll_callback, scroll);
    DECLARE_EVENT(mouseMove_callback, mouseMove);
    
    public:
        InputManager();


    private:
        static void InterceptRegisterRawInputDevices(PCRAWINPUTDEVICE rids, uint32_t numDevices, uint32_t ridSize);
        void SetupHooks(HWND hwnd);

        std::unique_ptr<DKUtil::Hook::RelHookHandle> _registerDevicesHook;

        static LRESULT CALLBACK _Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam);      
        static LRESULT CALLBACK _Hook_OnMouse(int code, WPARAM wParam, LPARAM lParam);      
        static LRESULT CALLBACK _Hook_WindowMessages(HWND hwnd, uint32_t uMsg, WPARAM wParam, LPARAM lParam);      
};

#undef DECLARE_KEYBOARD_EVENT
