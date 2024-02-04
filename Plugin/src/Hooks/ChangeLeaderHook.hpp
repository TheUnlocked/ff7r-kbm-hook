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
        CONFIG_OPTION(String,  MenuUp);
        CONFIG_OPTION(String,  MenuLeft);
        CONFIG_OPTION(String,  MenuDown);
        CONFIG_OPTION(String,  MenuRight);

        bool _prepared;
        uintptr_t _startAddress;
        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _hook;
};
