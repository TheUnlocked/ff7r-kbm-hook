#pragma once

#include "Hook.hpp"

class MotorcycleHook : public Hook<MotorcycleHook> {
    public:
        std::string Name() override {
            return "Motorcycle";
        }

        void Prepare() override;
        void Enable() override;
        void Disable() override;

        void RefreshConfig() override;

        bool ShouldEnable() override {
            return Config_EnableHook.get_data();
        }

    private:
        CONFIG_OPTION(Boolean, EnableHook);
        CONFIG_OPTION(Key, Accelerate);
        CONFIG_OPTION(Key, Brake);
        CONFIG_OPTION(Key, AttackLeft);
        CONFIG_OPTION(Key, AttackRight);
        CONFIG_OPTION(Key, Guard);
        CONFIG_OPTION(Key, Special);
        CONFIG_OPTION(Key, LongRange);

        char* _keybindsLocation;
        bool _isInMotorcycleGameMode = false;
        char _normalKeybinds[0xc0];

        void ApplyKeybinds();
        void RestoreKeybinds();

        static void TapKeybindsLocation(void* unk, uintptr_t param1);
        static void UpdateKeybinds(int* flags);
        
        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _tapKeybindsLocationHook;
        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _setModeFlagHook;
        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _clearModeFlagHook;
};
