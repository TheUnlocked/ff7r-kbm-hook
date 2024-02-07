#include "ChangeLeaderHook.hpp"


typedef byte func_ChangeLeader(void*, bool advanceForwards, bool /* ??? */, bool stopTime);
const std::uintptr_t ChangeLeaderAddress = dku::Hook::Module::get().base() + 0x9b67e0;
const std::uintptr_t PtrToChangeLeaderTarget = dku::Hook::Module::get().base() + 0x57b9220;

const std::uintptr_t ChangeLeaderCallerAddress = dku::Hook::Module::get().base() + 0x140ff90;
constexpr std::ptrdiff_t PrevLeaderBaseCallOffset = 0x1bc;
constexpr std::ptrdiff_t NextLeaderBaseCallOffset = 0x16e;
constexpr dku::Hook::Patch NOP_PATCH = { "\x90", 1 };

void ChangeLeaderHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_PrevLeader, "left");
    CONFIG_BIND(Config_NextLeader, "right");

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

void ChangeLeaderHook::on_KeyDown(int vkCode) {
    // TODO: detect if in combat

    if (vkCode == GetSingleton()->Config_PrevLeader.get_vkey_data()) {
        void* ChangeLeaderTarget = *reinterpret_cast<void**>(PtrToChangeLeaderTarget);
        ((func_ChangeLeader*) ChangeLeaderAddress)(ChangeLeaderTarget, false, false, true);
    }
    else if (vkCode == GetSingleton()->Config_NextLeader.get_vkey_data()) {
        void* ChangeLeaderTarget = *reinterpret_cast<void**>(PtrToChangeLeaderTarget);
        ((func_ChangeLeader*) ChangeLeaderAddress)(ChangeLeaderTarget, true, false, true);
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