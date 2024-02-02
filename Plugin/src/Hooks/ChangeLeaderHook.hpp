#pragma once

class ChangeLeaderHook {
    public:
        static void Prepare();
        static void Enable();
        static void Disable();

    private:
        inline static bool _prepared;
        inline static uintptr_t _startAddress;
        inline static std::unique_ptr<DKUtil::Hook::CaveHookHandle> _hook;
};