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
        CONFIG_OPTION(Boolean, ZExperiment_DisableTimeStop);

        std::unique_ptr<DKUtil::Hook::RelHookHandle> _changePrevHook;
        std::unique_ptr<DKUtil::Hook::RelHookHandle> _changeNextHook;

        static byte ChangeLeaderIntercept(uintptr_t self, bool advanceForwards, bool p3, bool stopTime);

        bool _pressedPrev;
        bool _pressedNext;

        static event_result on_keyDown(int vkCode);
        static event_result on_keyUp(int vkCode);
};
