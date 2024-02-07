#include "GeneralHook.hpp"


void GeneralHook::PrepareHook(_HookInfo* hookInfo) {
    hookInfo->hook->Prepare();
}

void GeneralHook::EnableHook(_HookInfo* hookInfo) {
    if (!hookInfo->enabled) {
        hookInfo->hook->Enable();
        hookInfo->enabled = true;
        INFO("{} enabled", hookInfo->hook->Name());
    }
}

void GeneralHook::DisableHook(_HookInfo* hookInfo) {
    if (hookInfo->enabled) {
        hookInfo->hook->Disable();
        hookInfo->enabled = false;
        INFO("{} disabled", hookInfo->hook->Name());
    }
}

void GeneralHook::Prepare() {
    CONFIG_BIND(Config_ReloadKey, "f10");

    for (auto &info : _hooks) {
        PrepareHook(info);
    }

    INFO("{} prepared", Name());

    _prepared = true;
}

void GeneralHook::RegisterHook(Hook<std::any>* hook) {
    auto info = new _HookInfo { .hook = hook, .enabled = false };

    if (_prepared) {
        PrepareHook(info);
    }

    if (_enabled) {
        if (hook->ShouldEnable()) {
            EnableHook(info);
        }
    }

    _hooks.push_back(info);
    INFO("{} registered", hook->Name());
}

void GeneralHook::ReloadConfig() {
    CONFIG_LOAD();

    for (auto info : _hooks) {
        if (info->hook->ShouldEnable()) {
            EnableHook(info);
        }
        else {
            DisableHook(info);
        }
    }

    INFO("Config reloaded");
}

void GeneralHook::on_KeyDown(int vkCode) {
    auto reloadKey = GetSingleton()->Config_ReloadKey.get_vkey_data();
    if (vkCode == reloadKey) {
        GetSingleton()->ReloadConfig();
    }
}

void GeneralHook::Enable() {
    for (auto info : _hooks) {
        if (info->hook->ShouldEnable()) {
            EnableHook(info);
        }
    }

    InputManager::GetSingleton()->register_on_keyDown(on_KeyDown);

    INFO("{} enabled", Name());

    _enabled = true;
}

void GeneralHook::Disable() {
    for (auto info : _hooks) {
        DisableHook(info);
    }

    InputManager::GetSingleton()->free_on_keyDown(on_KeyDown);

    INFO("{} disabled", Name());

    _enabled = false;
}
