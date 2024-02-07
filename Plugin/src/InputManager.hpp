#pragma once

class InputManager : public dku::Singleton<InputManager> {
    public:
        using keyboard_callback = void (*)(int vkCode);

        void register_on_keyDown(keyboard_callback callback);
        void free_on_keyDown(keyboard_callback callback);

        InputManager();

    private:
        std::vector<keyboard_callback> _keyDown_callbacks;

        static LRESULT CALLBACK _Hook_OnKeyboard(int code, WPARAM wParam, LPARAM lParam);      
        static LRESULT CALLBACK _Hook_OnMouse(int code, WPARAM wParam, LPARAM lParam);      
};
