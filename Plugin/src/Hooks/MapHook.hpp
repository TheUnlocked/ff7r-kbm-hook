#pragma once

#include "Hook.hpp"

class MapHook : public Hook<MapHook> {
    public:
        std::string Name() override {
            return "Map";
        }

        void Prepare() override;
        void Enable() override;
        void Disable() override;

        bool ShouldEnable() override {
            return Config_EnableHook.get_data();
        }

    private:
        CONFIG_OPTION(Boolean, EnableHook);
        CONFIG_OPTION(Boolean, LockMouse);
        CONFIG_OPTION(Double, ZoomSensitivity);

        std::unique_ptr<DKUtil::Hook::CaveHookHandle> _hook;

        static void SetMapCursorPosition(uintptr_t param1);

        float _scrollTicks = 0;
        std::chrono::system_clock::time_point _lastScrollTime = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point _lastMapTime = std::chrono::system_clock::now();

        static event_result on_scroll(int16_t delta);
};
