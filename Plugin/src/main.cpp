#include "Settings.hpp"
#include "Hooks/GeneralHook.hpp"
#include "Hooks/ChangeLeaderHook.hpp"

using namespace DKUtil::Alias;

void Install() {
    dku::Hook::Trampoline::AllocTrampoline(1 << 6);

	auto hooks = GeneralHook::GetSingleton();

	hooks->RegisterHook<ChangeLeaderHook>();
	hooks->Prepare();

	CONFIG_SETUP();
    
    hooks->Enable();
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

        Install();
    }

    return TRUE;
}