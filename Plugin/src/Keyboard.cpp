#include "Keyboard.hpp"

const std::map<std::string, int> keyMap = {
    { "lbutton", VK_LBUTTON },
    { "rbutton", VK_RBUTTON },
    { "mbutton", VK_MBUTTON },
    { "xbutton1", VK_XBUTTON1 },
    { "xbutton2", VK_XBUTTON2 },
    { "back", VK_BACK }, { "backspace", VK_BACK },
    { "tab", VK_TAB },
    { "clear", VK_CLEAR },
    { "return", VK_RETURN }, { "enter", VK_RETURN },
    { "shift", VK_SHIFT },
    { "control", VK_CONTROL }, { "ctrl", VK_CONTROL },
    { "alt", VK_MENU }, // excluding menu since VK_APPS is more commonly called "menu"
    { "pause", VK_PAUSE }, { "break", VK_PAUSE }, { "pausebreak", VK_PAUSE }, 
    { "capital", VK_CAPITAL }, { "capslock", VK_CAPITAL },
    { "escape", VK_ESCAPE }, { "esc", VK_ESCAPE },
    { "space", VK_SPACE },
    { "prior", VK_PRIOR }, { "pageup", VK_PRIOR },
    { "next", VK_NEXT }, { "pagedown", VK_NEXT },
    { "left", VK_LEFT },
    { "up", VK_UP },
    { "right", VK_RIGHT },
    { "down", VK_DOWN },
    { "snapshot", VK_SNAPSHOT }, { "printscreen", VK_SNAPSHOT },
    { "insert", VK_INSERT },
    { "delete", VK_DELETE },
    { "help", VK_HELP },
    { "lwin", VK_LWIN }, { "win", VK_LWIN },
    { "rwin", VK_RWIN },
    { "apps", VK_APPS }, { "menu", VK_APPS },
    { "numpad0", VK_NUMPAD0 },
    { "numpad1", VK_NUMPAD1 },
    { "numpad2", VK_NUMPAD2 },
    { "numpad3", VK_NUMPAD3 },
    { "numpad4", VK_NUMPAD4 },
    { "numpad5", VK_NUMPAD5 },
    { "numpad6", VK_NUMPAD6 },
    { "numpad7", VK_NUMPAD7 },
    { "numpad8", VK_NUMPAD8 },
    { "numpad9", VK_NUMPAD9 },
    { "multiply", VK_MULTIPLY },
    { "add", VK_ADD },
    { "subtract", VK_SUBTRACT },
    { "divide", VK_DIVIDE },
    { "f1", VK_F1 },
    { "f2", VK_F2 },
    { "f3", VK_F3 },
    { "f4", VK_F4 },
    { "f5", VK_F5 },
    { "f6", VK_F6 },
    { "f7", VK_F7 },
    { "f8", VK_F8 },
    { "f9", VK_F9 },
    { "f10", VK_F10 },
    { "f11", VK_F11 },
    { "f12", VK_F12 },
    { "f13", VK_F13 },
    { "f14", VK_F14 },
    { "f15", VK_F15 },
    { "f16", VK_F16 },
    { "f17", VK_F17 },
    { "f18", VK_F18 },
    { "f19", VK_F19 },
    { "f20", VK_F20 },
    { "f21", VK_F21 },
    { "f22", VK_F22 },
    { "f23", VK_F23 },
    { "f24", VK_F24 },
    { "numlock", VK_NUMLOCK },
    { "scroll", VK_SCROLL }, { "scrolllock", VK_SCROLL },
    { "lshift", VK_LSHIFT },
    { "rshift", VK_RSHIFT },
    { "lcontrol", VK_LCONTROL }, { "lctrl", VK_LCONTROL },
    { "rcontrol", VK_RCONTROL }, { "rctrl", VK_RCONTROL },
    { "lmenu", VK_LMENU }, { "lalt", VK_LMENU },
    { "rmenu", VK_RMENU }, { "ralt", VK_RMENU },
};

std::optional<int> KeyNameToVKey(std::string keyName) {
    if (keyName.length() == 1) {
        // 0-9, A-Z, a-z
        unsigned char ch = keyName[0];
        if (ch - '0' < 10) {
            return ch;
        }
        else if (ch - 'a' < 26) {
            return ch - 'a' + 'A';
        }
        return {}; // there are no other 1-character options
    }

    auto found = keyMap.find(keyName);
    if (found == keyMap.end()) {
        return {};
    }
    return found->second;
}
