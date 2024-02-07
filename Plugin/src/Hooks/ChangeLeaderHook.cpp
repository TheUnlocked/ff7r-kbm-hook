#include "ChangeLeaderHook.hpp"


const std::uintptr_t base = dku::Hook::Module::get().base();

template<typename T>
T* ReadStatic(std::ptrdiff_t offset) {
    return reinterpret_cast<T*>(base + offset);
}

typedef byte func_ChangeLeader(uintptr_t, bool advanceForwards, bool /* ??? */, bool stopTime);

enum GameplayMode : byte {
    Unknown0, // combat/loading?
    Unknown1, // world?
    Unknown2, // menu/cutscene?
    Unknown3, // ???
    Unknown4, // ???
};
typedef GameplayMode func_GetGameplayMode(uintptr_t);

const auto ChangeLeader = ReadStatic<func_ChangeLeader>(0x9b67e0);
const auto GetGameplayMode = ReadStatic<func_GetGameplayMode>(0x28c8060);

uintptr_t ChangeLeaderArgument() {
    return *ReadStatic<uintptr_t>(0x57b9220);
}

uintptr_t GetGameplayModeArgument() {
    // Logic is basically just extracted from ghidra
    auto iVar7 = *ReadStatic<int>(0x5999f58 + 0x2f8);
    if (iVar7 < 0 || iVar7 >= *ReadStatic<int>(0x538d41c)) {
        return 0;
    }
    auto PTR_14538d410 = *ReadStatic<uintptr_t>(0x538d410);
    auto lVar8 = reinterpret_cast<uintptr_t*>(PTR_14538d410 + (iVar7 * 0x18));
    if (lVar8 == nullptr) {
        return 0;
    }
    return *lVar8;
}

const std::uintptr_t ChangeLeaderCallerAddress = base + 0x140ff90;
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
        std::make_pair(PrevLeaderBaseCallOffset, PrevLeaderBaseCallOffset + 0x5),
        NOP_PATCH
    );

    _changeNextHook = dku::Hook::AddASMPatch(
        ChangeLeaderCallerAddress,
        std::make_pair(NextLeaderBaseCallOffset, NextLeaderBaseCallOffset + 0x5),
        NOP_PATCH
    );
}

bool CanChangeLeader() {
    // TODO: Does not work perfectly, needs more investigation
    auto arg = GetGameplayModeArgument();
    if (arg == 0) {
        return false;
    }
    auto mode = GetGameplayMode(arg);
    if (mode == Unknown1 || mode == Unknown2) {
        return false;
    }
    return true;
}

void ChangeLeaderHook::TryChangeLeader(bool forwards) {
    bool anyTime = GetSingleton()->Config_ZExperiment_AllowChangingLeaderOutOfCombat.get_data();
    if (anyTime || CanChangeLeader()) {
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