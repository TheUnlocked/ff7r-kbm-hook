#include "ChangeLeaderHook.hpp"


typedef byte func_ChangeLeader(uintptr_t, bool advanceForwards, bool /* ??? */, bool stopTime);

const auto ChangeLeader = Memory::deref_static<func_ChangeLeader>(0x9b67e0);
const std::uintptr_t ChangeLeaderCallerAddress = Memory::get_code_address(0x140ff90);
constexpr std::ptrdiff_t PrevLeaderBaseCallOffset = 0x1bc;
constexpr std::ptrdiff_t NextLeaderBaseCallOffset = 0x16e;

byte ChangeLeaderHook::ChangeLeaderIntercept(uintptr_t obj, bool advanceForwards, bool p3, bool stopTime) {
    auto self = GetSingleton();

    auto disableStopTime = self->Config_ZExperiment_DisableTimeStop.get_data();
    if (!advanceForwards) {
        if (!self->_pressedPrev) {
            return 0;
        }
        self->_pressedPrev = false;
    }
    else {
        if (!self->_pressedNext) {
            return 0;
        }
        self->_pressedNext = false;
    }

    return ChangeLeader(obj, advanceForwards, p3, disableStopTime ? false : stopTime);
}

void ChangeLeaderHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_PrevLeader, "left");
    CONFIG_BIND(Config_NextLeader, "right");
    CONFIG_BIND(Config_ZExperiment_DisableTimeStop, false);

    _changePrevHook = dku::Hook::AddRelHook<5, true>(
        ChangeLeaderCallerAddress + PrevLeaderBaseCallOffset,
        AsAddress(&ChangeLeaderIntercept)
    );

    _changeNextHook = dku::Hook::AddRelHook<5, true>(
        ChangeLeaderCallerAddress + NextLeaderBaseCallOffset,
        AsAddress(&ChangeLeaderIntercept)
    );
}

void ChangeLeaderHook::on_keyDown(int vkCode) {
    auto self = GetSingleton();

    int targetVkey = 0;
    if (vkCode == self->Config_PrevLeader.get_vkey_data()) {
        targetVkey = VK_LEFT;
        self->_pressedPrev = true;
    }
    else if (vkCode == self->Config_NextLeader.get_vkey_data()) {
        targetVkey = VK_RIGHT;
        self->_pressedNext = true;
    }
    
    if (targetVkey != 0) {
        INPUT ip {
            INPUT_KEYBOARD,
            { .ki = {
                .wVk = (WORD)targetVkey,
                .wScan = (WORD)MapVirtualKeyA(targetVkey, MAPVK_VK_TO_VSC_EX)
            } }
        };
        SendInput(1, &ip, sizeof(INPUT));
    }
}

void ChangeLeaderHook::on_keyUp(int vkCode) {
    auto self = GetSingleton();
    
    int targetVkey = 0;
    if (vkCode == self->Config_PrevLeader.get_vkey_data()) {
        targetVkey = VK_LEFT;
        self->_pressedPrev = false;
    }
    else if (vkCode == self->Config_NextLeader.get_vkey_data()) {
        targetVkey = VK_RIGHT;
        self->_pressedNext = false;
    }
    
    if (targetVkey != 0) {
        INPUT ip {
            INPUT_KEYBOARD,
            { .ki = {
                .wVk = (WORD)targetVkey,
                .wScan = (WORD)MapVirtualKeyA(targetVkey, MAPVK_VK_TO_VSC_EX),
                .dwFlags = KEYEVENTF_KEYUP
            } }
        };
        SendInput(1, &ip, sizeof(INPUT));
    }
}

void ChangeLeaderHook::Enable() {
    _changePrevHook->Enable();
    _changeNextHook->Enable();

    InputManager::GetSingleton()->register_on_keyDown(on_keyDown);
    InputManager::GetSingleton()->register_on_keyUp(on_keyUp);
}

void ChangeLeaderHook::Disable() {
    _changePrevHook->Disable();
    _changeNextHook->Disable();

    InputManager::GetSingleton()->free_on_keyDown(on_keyDown);
    InputManager::GetSingleton()->free_on_keyUp(on_keyUp);
}