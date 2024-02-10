#pragma once

#include "Hook.hpp"

class MovementHook : public Hook<MovementHook> {
    public:
        std::string Name() override {
            return "Movement";
        }

        void Prepare() override;
        void Enable() override;
        void Disable() override;

        bool ShouldEnable() override {
            return Config_EnableHook.get_data();
        }

    private:
        CONFIG_OPTION(Boolean, EnableHook);
        CONFIG_OPTION(Key, WalkKey);
        CONFIG_OPTION(Boolean, ToggleWalk);

        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _hook;

        static void WalkInjection(float* joystickXY);

        bool _walkToggledOn = false;

        static event_result on_keyDown(int vkCode);
};
