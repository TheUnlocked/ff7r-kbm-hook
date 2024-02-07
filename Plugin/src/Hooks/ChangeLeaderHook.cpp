#include "ChangeLeaderHook.hpp"


typedef byte func_ChangeLeader(uintptr_t, bool advanceForwards, bool /* ??? */, bool stopTime);

const auto ChangeLeader = Memory::deref_static<func_ChangeLeader>(0x9b67e0);

uintptr_t ChangeLeaderArgument() {
    return Memory::deref_static<uintptr_t>(0x57b9220);
}

const std::uintptr_t ChangeLeaderCallerAddress = Memory::get_code_address(0x140ff90);
constexpr std::ptrdiff_t PrevLeaderBaseCallOffset = 0x1bc;
constexpr std::ptrdiff_t NextLeaderBaseCallOffset = 0x16e;
constexpr dku::Hook::Patch NOP_PATCH = { "\x90", 1 };

void ChangeLeaderHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_PrevLeader, "left");
    CONFIG_BIND(Config_NextLeader, "right");
    CONFIG_BIND(Config_ZExperiment_AllowChangingLeaderOutOfCombat, false);
    CONFIG_BIND(Config_ZExperiment_DisableTimeStop, false);

    _changePrevHook = dku::Hook::AddASMPatch(
        ChangeLeaderCallerAddress,
        std::make_pair(PrevLeaderBaseCallOffset, PrevLeaderBaseCallOffset + 5),
        NOP_PATCH
    );

    _changeNextHook = dku::Hook::AddASMPatch(
        ChangeLeaderCallerAddress,
        std::make_pair(NextLeaderBaseCallOffset, NextLeaderBaseCallOffset + 5),
        NOP_PATCH
    );
}

bool IsInCombat() {
    return Memory::deref<bool>(Memory::deref_static<uintptr_t>(0x5999f58), 0x348);
}

void ChangeLeaderHook::TryChangeLeader(bool forwards) {
    bool anyTime = GetSingleton()->Config_ZExperiment_AllowChangingLeaderOutOfCombat.get_data();
    if (anyTime || IsInCombat()) {
        bool timeStop = !GetSingleton()->Config_ZExperiment_DisableTimeStop.get_data();
        ChangeLeader(ChangeLeaderArgument(), forwards, false, timeStop);
    }
}

void ChangeLeaderHook::on_KeyDown(int vkCode) {
    if (vkCode == GetSingleton()->Config_PrevLeader.get_vkey_data()) {
        TryChangeLeader(false);
    }
    else if (vkCode == GetSingleton()->Config_NextLeader.get_vkey_data()) {
        TryChangeLeader(true);
    }
}

void ChangeLeaderHook::Enable() {
    _changePrevHook->Enable();
    _changeNextHook->Enable();

    InputManager::GetSingleton()->register_on_keyDown(on_KeyDown);
}

void ChangeLeaderHook::Disable() {
    _changePrevHook->Disable();
    _changeNextHook->Disable();

    InputManager::GetSingleton()->free_on_keyDown(on_KeyDown);
}