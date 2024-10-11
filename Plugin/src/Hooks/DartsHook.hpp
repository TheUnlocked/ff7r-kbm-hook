#pragma once

#include "Hook.hpp"

class DartsHook : public Hook<DartsHook> {
    public:
        std::string Name() override {
            return "Darts";
        }

        void Prepare() override;
        void Enable() override;
        void Disable() override;

        bool ShouldEnable() override {
            return Config_EnableHook.get_data();
        }

    private:
        CONFIG_OPTION(Boolean, EnableHook);
        CONFIG_OPTION(Double, MouseSensitivity);
        // CONFIG_OPTION(Double, ReturnSpeed);

        std::unique_ptr<DKUtil::Hook::RelHookHandle> _updateReticleHook;
        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _resetReticleHook;

        static void UpdateReticlePositionIntercept(uintptr_t self, float deltaT);
        static void ResetReticleInjection();

        bool firstDartsTick = true;
};
