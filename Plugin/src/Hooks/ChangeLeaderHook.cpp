#include "ChangeLeaderHook.hpp"


typedef byte func_ChangeLeader(uintptr_t, bool advanceForwards, bool /* ??? */, bool stopTime);

const auto ChangeLeader = reinterpret_cast<func_ChangeLeader*>(
    AsAddress(dku::Hook::search_pattern<
        "41 57 "
        "48 8d ac 24 60 ec ff ff "
        "b8 a0 14 00 00"
    >()) - 0xe // 09b67ee
);
const std::uintptr_t ChangeLeaderNextCallerAddress =
    AsAddress(dku::Hook::search_pattern<
        "45 0f b6 cf "
        "45 33 c0 "
        "41 0f b6 d7"
    >()) + 0xb; // 14100f3
const std::uintptr_t ChangeLeaderPrevCallerAddress = ChangeLeaderNextCallerAddress + 0x4e;

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
        ChangeLeaderNextCallerAddress,
        AsAddress(&ChangeLeaderIntercept)
    );

    _changeNextHook = dku::Hook::AddRelHook<5, true>(
        ChangeLeaderPrevCallerAddress,
        AsAddress(&ChangeLeaderIntercept)
    );
}

event_result ChangeLeaderHook::on_keyDown(int vkCode) {
    auto self = GetSingleton();

    int targetVkey = 0;

    // 20ms should be long enough for an update cycle, but is short enough to avoid the case where a user holds
    // a switch key when switching is disallowed, then presses an unrelated key triggering the switch unexpectedly. 
    if (vkCode == self->Config_PrevLeader.get_vkey_data()) {
        targetVkey = VK_LEFT;
        self->_pressedPrev = true;
        std::thread([self] {
            std::this_thread::sleep_for(20ms);
            self->_pressedPrev = false;
        }).detach();
    }
    else if (vkCode == self->Config_NextLeader.get_vkey_data()) {
        targetVkey = VK_RIGHT;
        self->_pressedNext = true;
        std::thread([self] {
            std::this_thread::sleep_for(20ms);
            self->_pressedNext = false;
        }).detach();
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
    return Continue;
}

event_result ChangeLeaderHook::on_keyUp(int vkCode) {
    auto self = GetSingleton();
    
    int targetVkey = 0;
    if (vkCode == self->Config_PrevLeader.get_vkey_data()) {
        targetVkey = VK_LEFT;
    }
    else if (vkCode == self->Config_NextLeader.get_vkey_data()) {
        targetVkey = VK_RIGHT;
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
    return Continue;
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