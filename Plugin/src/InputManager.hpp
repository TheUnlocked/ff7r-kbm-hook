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

    DECLARE_EVENT(keyboard_callback, keyDown);
    DECLARE_EVENT(keyboard_callback, keyUp);
    DECLARE_EVENT(scroll_callback, scroll);
    
    public:
        InputManager();


    private:
        static LRESULT CALLBACK _Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam);      
        static LRESULT CALLBACK _Hook_OnMouse(int code, WPARAM wParam, LPARAM lParam);      
};

#undef DECLARE_KEYBOARD_EVENT
