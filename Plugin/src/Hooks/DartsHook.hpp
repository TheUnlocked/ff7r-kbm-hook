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
        CONFIG_OPTION(Double, ReturnSpeed);
        CONFIG_OPTION(Boolean, ZExperiment_Alternative);

        std::unique_ptr<DKUtil::Hook::RelHookHandle> _updateReticleHook;
        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _resetReticleHook;

        static void UpdateReticlePositionIntercept(uintptr_t obj, float deltaT);
        static void ResetReticleInjection(uintptr_t obj);

        bool firstDartsTick = true;

        static event_result on_mouseMove(int32_t xDelta, int32_t yDelta);
};
