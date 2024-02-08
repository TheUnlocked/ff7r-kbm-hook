#pragma once

#define DECLARE_KEYBOARD_EVENT(name)                                \
    public:                                                         \
        void register_on_## name ##(keyboard_callback callback);    \
        void free_on_## name ##(keyboard_callback callback);        \
    private:                                                        \
        std::vector<keyboard_callback> _## name ##_callbacks;

class InputManager : public dku::Singleton<InputManager> {
    using keyboard_callback = void (*)(int vkCode);

    DECLARE_KEYBOARD_EVENT(keyDown);
    DECLARE_KEYBOARD_EVENT(keyUp);

    public:
        InputManager();

    private:
        static LRESULT CALLBACK _Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam);      
        static LRESULT CALLBACK _Hook_OnMouse(int code, WPARAM wParam, LPARAM lParam);      
};

#undef DECLARE_KEYBOARD_EVENT
