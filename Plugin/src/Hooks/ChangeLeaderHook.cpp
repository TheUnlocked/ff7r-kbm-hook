#include "ChangeLeaderHook.hpp"

#define PATCH(bytes) (dku::Hook::Patch { bytes, sizeof(bytes) - 1 })

// Return whether to run the original function
bool __cdecl ChangeLeaderHook::Hook_ChangeLeader() {
    auto hook = GetSingleton();

    for (auto key : hook->Config_MenuKeys.get_vkey_collection()) {
        if (GetAsyncKeyState(key) < 0) {
            return true;
        }
    }

    return false;
}

std::uintptr_t ChangeLeaderHook::InterceptStartAddress() {
    auto* addr = dku::Hook::Assembly::search_pattern<
        "41 57 "                    // PUSH       R15
        "48 8d ac 24 60 ec ff ff"   // LEA RBP=>[RSP + -0x13a0]
    >();
    return AsAddress(addr) - 0x0e;
}

constexpr std::ptrdiff_t InterceptLength = 0x100;

void ChangeLeaderHook::Prepare() {
    if (_prepared) {
        return;
    }

    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_MenuKeys, "up", "left", "down", "right");

    _startAddress = InterceptStartAddress();

    _hook = dku::Hook::AddCaveHook(
        _startAddress,
        std::make_pair(0, InterceptLength),
        FUNC_INFO(Hook_ChangeLeader),
        PATCH(
            "\x48\x83\xec\x08"  // sub rsp,8    -- need to align the stack

            "\x51"              // push rcx     -- save argument
            "\x52"              // push rdx     -- save argument
            "\x41\x50"          // push r8      -- save argument
            "\x41\x51"          // push r9      -- save argument
        ), 
        PATCH(
            "\x41\x59"          // pop r9       -- restore argument
            "\x41\x58"          // pop r8       -- restore argument
            "\x5a"              // pop rdx      -- restore argument
            "\x59"              // pop rcx      -- restore argument

            "\x48\x83\xc4\x08"  // add rsp,8    -- restore the stack
            "\x48\x85\xc0"      // test rax,rax -- set ZF if rax & rax = 0 (i.e. if rax = 0)
            "\x75\x01"          // jnz short 2  -- if ZF not set, jump two bytes ahead (i.e. right after ret)
            "\xc3"              // ret          -- return from the original function call
            
            "\x48\x89\x5c\x24\x10" // first instruction of original function
        ),
        // It would be nice to be able to put the original function right after the epilogue,
        // but it has relative jumps that get messed up if we do that.
        dku::Hook::HookFlag::kNoFlag
    );

    _prepared = true;
}

void ChangeLeaderHook::Enable() {
    _hook->Enable();

    // Restore the original function (the first 5-byte instruction is part of the epilogue)
    // This needs to be in the exact same place as it was before to preserve relative jumps.
    dku::Hook::WriteData(
        _startAddress + 5,
        std::span(_hook->OldBytes).subspan(5).data(),
        InterceptLength - 5,
        false
    );
}

void ChangeLeaderHook::Disable() {
    _hook->Disable();
}