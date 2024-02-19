#include "MotorcycleHook.hpp"

#define PATCH(code) dku::Hook::Patch { code, sizeof(code) - 1 }

// There's a big list of flags. Not entirely sure what they represent, but I have some decent guesses for some.
enum ModeFlagType : int {
    GAMEPLAY_TYPE = 0, // 0 for normal gameplay, 1 sometimes(??), 2 in main menu
    UNUSUAL_GAME_MODE = 33, // appears to be 1 during the motorcycle game and in the main menu (maybe elsewhere too?)
    CUTSCENE = 45,
    PAUSED = 69,
    // Both of these flags tend to be the same value and are correlated with there being a loading screen.
    // Not sure what they precisely mean, but I figured I'd document them here anyways.
    LOADING1 = 37,
    LOADING2 = 70,
};

namespace Keybind {
    enum Keybind {
        Confirm = 0,
        Cancel,
        Remove,
        EquipMateria,
        IconKey,
        SetForAll,
        SelectController,
        MapOfMidgar,
        CurrentPosition,
        Sort,
        PrevGroup,
        NextGroup,
        MenuUp,
        MenuDown,
        MenuLeft,
        MenuRight,
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        LookUp,
        LookDown,
        LookLeft,
        LookRight,
        Shortcut1,
        Shortcut2,
        Shortcut3,
        Shortcut4,
        AtbBoost,
        NinjaCannonball,
        ChangeLeaderToAlly2,
        ChangeLeaderToAlly1,
        Pause,
        Attack,
        OpenCommandsMenu,
        Interact,
        PerformUniqueAbility,
        DashHeld,
        DashHeld2, // doesn't appear in the menu, appears to be another dash key?
        DashToggle,
        Guard,
        Map,
        ViewEnemyIntel,
        Evade,
        ToggleLockOn,
        ToggleMinimap,
        IssueCommandToAlly1,
        IssueCommandToAlly2,
        OpenShortcutMenu,
        Unk1,
        Unk2,
        ResetCamera,
        SelectStrongerSpell,
        SelectWeakerSpell,
        Magnify,
        SelectLockOnTargetLeft,
        SelectLockOnTargetRight,
        ScrollUp,
        ScrollDown,
        ShowStats,
    };
}


void MotorcycleHook::ApplyKeybinds() {
    // save original keybinds
    std::memcpy(_normalKeybinds, _keybindsLocation, sizeof(_normalKeybinds));

    auto accelerate  = Config_Accelerate.get_vkey_data();
    auto brake       = Config_Brake.get_vkey_data();
    auto attackLeft  = Config_AttackLeft.get_vkey_data();
    auto attackRight = Config_AttackRight.get_vkey_data();
    auto guard       = Config_Guard.get_vkey_data();
    auto special     = Config_Special.get_vkey_data();
    auto longRange   = Config_LongRange.get_vkey_data();

    auto ApplyBind = [&](size_t keyIdx, int to) {
        if (to) {
            bool isMouseBtn;
            switch (to) {
                case VK_LBUTTON:
                case VK_RBUTTON:
                case VK_MBUTTON:
                case VK_XBUTTON1:
                case VK_XBUTTON2:
                    isMouseBtn = true;
                    break;
                default:
                    isMouseBtn = false;
                    break;
            }
            if (isMouseBtn) {
                _keybindsLocation[keyIdx] = 0;
                _keybindsLocation[keyIdx + 0x40] = 0;
                _keybindsLocation[keyIdx + 0x80] = to;
            }
            else {
                _keybindsLocation[keyIdx] = to;
                _keybindsLocation[keyIdx + 0x40] = 0;
                _keybindsLocation[keyIdx + 0x80] = 0;
            }
        }
    };

    ApplyBind(Keybind::IssueCommandToAlly2, accelerate);
    ApplyBind(Keybind::IssueCommandToAlly1, brake);
    ApplyBind(Keybind::Attack, attackLeft);
    ApplyBind(Keybind::Evade, attackRight);
    ApplyBind(Keybind::Guard, guard);
    ApplyBind(Keybind::PerformUniqueAbility, special);
    ApplyBind(Keybind::OpenShortcutMenu, longRange);

    INFO("Applied motorcycle keybinds");
}

void MotorcycleHook::RestoreKeybinds() {
    std::memcpy(_keybindsLocation, _normalKeybinds, sizeof(_normalKeybinds));
    INFO("Restored original keybinds");
}

void MotorcycleHook::UpdateKeybinds(int* flags) {
    auto self = GetSingleton();

    if (flags[GAMEPLAY_TYPE] == 1 && flags[UNUSUAL_GAME_MODE] == 1) {
        // From what I can tell this combination of values only occurs in the motorcycle minigame, but I could be wrong.
        // I haven't tested it in darts or the reactor 5 bridge puzzles/sector 6 claw puzzles so it may apply to that too.
        // Hopefully this hook shouldn't impact those minigames though, even if they do have the same flags.
        if (!self->_isInMotorcycleGameMode) {
            self->_isInMotorcycleGameMode = true;
            INFO("Switched to motorcycle game, setting keybinds");
            self->ApplyKeybinds();
        }
    }
    else if (self->_isInMotorcycleGameMode) {
        self->_isInMotorcycleGameMode = false;
        INFO("Switched out of motorcycle game, restoring keybinds");
        self->RestoreKeybinds();
    }
}

auto OriginalFunctionAtTapKeybindsLocation = reinterpret_cast<void(*)(void*)>(
    AsAddress(dku::Hook::search_pattern<
        "4c 8b dc "
        "41 56 "
        "48 83 ec 50"
    >()) // 1fafc10
);

void MotorcycleHook::TapKeybindsLocation(void* unk, uintptr_t param1) {
    auto self = GetSingleton();
    
    self->_keybindsLocation = &Memory::deref<char>(param1, 0x250);
    INFO("Found keybinds address: {:X}", AsAddress(self->_keybindsLocation));

    // discard hook
    self->_tapKeybindsLocationHook->Disable();
    self->_tapKeybindsLocationHook = nullptr;

    // Call original function
    OriginalFunctionAtTapKeybindsLocation(unk);
}

void MotorcycleHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_Accelerate, "w");
    CONFIG_BIND(Config_Brake, "s");
    CONFIG_BIND(Config_AttackLeft, "lbutton");
    CONFIG_BIND(Config_AttackRight, "rbutton");
    CONFIG_BIND(Config_Guard, "");
    CONFIG_BIND(Config_Special, "");
    CONFIG_BIND(Config_LongRange, "r");

    _tapKeybindsLocationHook = dku::Hook::AddCaveHook(
         AsAddress(dku::Hook::search_pattern<
            "e8 ?? ?? ?? ?? "
            "48 8b 44 24 70 "
            "48 85 c0 "
            "74 10"
        >()), // 1428fb0
        std::make_pair(0, 5),
        FUNC_INFO(TapKeybindsLocation),
        PATCH(
            "\x48\x89\xf2" // mov rdx,rsi
        )
    );
    _tapKeybindsLocationHook->Enable();

    _setModeFlagHook = dku::Hook::AddCaveHook(
        AsAddress(dku::Hook::search_pattern<
            "48 89 9c ee 24 65 10 01"
        >()), // 1138b7a
        std::make_pair(0, 8),
        FUNC_INFO(UpdateKeybinds),
        PATCH(
            "\x48\x8d\x8e\xb4\x63\x10\x01" // lea rcx,[rsi+0x11063b4]
        ),
        PATCH(""),
        HookFlag::kRestoreBeforeProlog
    );

    _clearModeFlagHook = dku::Hook::AddCaveHook(
        AsAddress(dku::Hook::search_pattern<
            "4a 89 84 f3 24 65 10 01"
        >()), // 1138c77
        std::make_pair(0, 8),
        FUNC_INFO(UpdateKeybinds),
        PATCH(
            "\x48\x8d\x8b\xb4\x63\x10\x01" // lea rcx,[rbx+0x11063b4]
        ),
        PATCH(""),
        HookFlag::kRestoreBeforeProlog
    );
}

void MotorcycleHook::Enable() {
    // _setModeFlagHook->Enable();
    _clearModeFlagHook->Enable();
}

void MotorcycleHook::Disable() {
    // _setModeFlagHook->Disable();
    _clearModeFlagHook->Disable();
    
    if (_isInMotorcycleGameMode) {
        INFO("Disabled hook while in motorcycle game, need to restore keybinds");
        RestoreKeybinds();
    }
}

void MotorcycleHook::RefreshConfig() {
    if (_isInMotorcycleGameMode) {
        INFO("Config updated while in motorcycle game, need to re-apply keybinds");
        RestoreKeybinds();
        ApplyKeybinds();
    }
}
