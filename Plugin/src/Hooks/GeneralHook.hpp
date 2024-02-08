#pragma once

#include "Hook.hpp"

class GeneralHook : public Hook<GeneralHook> {
    public:
        std::string Name() override {
            return "General";
        }

        void Prepare() override;
        void Enable() override;
        void Disable() override;

        template<IsHook T>
        void RegisterHook() {
            RegisterHook(reinterpret_cast<Hook<std::any>*>(T::GetSingleton()));
        }
        void RegisterHook(Hook<std::any>* hook);

    private:
        CONFIG_OPTION(Key, ReloadKey);

        struct _HookInfo {
            Hook<std::any>* hook;
            bool enabled;
        };

        void PrepareHook(_HookInfo* hookInfo);
        void EnableHook(_HookInfo* hookInfo);
        void DisableHook(_HookInfo* hookInfo);
        void ReloadConfig();

        bool _prepared;
        bool _enabled;
        std::vector<_HookInfo*> _hooks;

        static void on_keyDown(int vkCode);
};
