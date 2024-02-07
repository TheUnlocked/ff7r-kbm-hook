#pragma once

#include "Hook.hpp"

class ChangeLeaderHook : public Hook<ChangeLeaderHook> {
    public:
        std::string Name() override {
            return "ChangeLeader";
        }

        void Prepare() override;
        void Enable() override;
        void Disable() override;

        bool ShouldEnable() override {
            return Config_EnableHook.get_data();
        }

    private:
        CONFIG_OPTION(Boolean, EnableHook);
        CONFIG_OPTION(Key, PrevLeader);
        CONFIG_OPTION(Key, NextLeader);
        CONFIG_OPTION(Boolean, ZExperiment_AllowChangingLeaderOutOfCombat);
        CONFIG_OPTION(Boolean, ZExperiment_DisableTimeStop);

        std::unique_ptr<DKUtil::Hook::ASMPatchHandle> _changePrevHook;
        std::unique_ptr<DKUtil::Hook::ASMPatchHandle> _changeNextHook;

        static void on_KeyDown(int vkCode);
        static void TryChangeLeader(bool forwards);
};
