#include "Settings.hpp"
#include "Hooks/GeneralHook.hpp"
#include "Hooks/ChangeLeaderHook.hpp"
#include "Hooks/MapHook.hpp"
#include "Hooks/MovementHook.hpp"
#include "Hooks/MotorcycleHook.hpp"
#include "Hooks/DartsHook.hpp"

using namespace DKUtil::Alias;

void Install() {
    dku::Hook::Trampoline::AllocTrampoline(1 << 9);

    // Force the input manager to pre-load since it has to set up hooks during game initialization.
    InputManager::GetSingleton();

    auto hooks = GeneralHook::GetSingleton();

    hooks->RegisterHook<ChangeLeaderHook>();
    hooks->RegisterHook<MapHook>();
    hooks->RegisterHook<MovementHook>();
    hooks->RegisterHook<MotorcycleHook>();
    hooks->RegisterHook<DartsHook>();
    hooks->Prepare();

    CONFIG_SETUP();
    
    hooks->Enable();
}

extern "C" {
    __declspec(dllexport) void __cdecl Init() {
        Install();
    }
}


BOOL APIENTRY DllMain(HMODULE a_hModule, DWORD a_ul_reason_for_call, LPVOID a_lpReserved) {
    if (a_ul_reason_for_call == DLL_PROCESS_ATTACH) {
#ifndef NDEBUG
        while (!IsDebuggerPresent()) {
            Sleep(100);
        }
#endif

        dku::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
        INFO("game type : {}", dku::Hook::GetProcessName());

        DllState::hmodule = a_hModule;
        INFO("module handle ptr: {}", (void*)a_hModule);

        // Install();
    }

    return TRUE;
}