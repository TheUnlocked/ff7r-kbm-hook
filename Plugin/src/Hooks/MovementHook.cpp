#include "MovementHook.hpp"

#define PATCH(code) dku::Hook::Patch { code, sizeof(code) - 1 }

// Unclear on the actual thresholds, but 0.4 seems to be within the range for the slowest walking.
// There are higher values which transition into a fast walk before Cloud switches to jogging.
// I'm guessing the thresholds are approximately 0 - 0.3 - 0.5 - 0.66 - 1, but even within each zone
// there is some amount of variance in actual speed (except the 0 - 0.3 deadzone)
constexpr float WALK_SPEED = 0.4f;

// This is directly after the call where the raw joystick values are fetched
const auto WalkInjectionSite =
    AsAddress(dku::Hook::search_pattern<
        "f3 44 0f 10 55 40 "
        "f3 0f 10 8b 18 07 00 00"
    >()); // 15e8563

event_result MovementHook::on_keyDown(int vkCode) {
    auto self = GetSingleton();

    if (vkCode == self->Config_WalkKey.get_vkey_data()) {
        if (self->Config_ToggleWalk.get_data()) {
            self->_walkToggledOn = !self->_walkToggledOn;
            INFO("Walk toggled -> {}", self->_walkToggledOn);
        }
        else {
            self->_walkToggledOn = false;
        }
    }

    return Continue;
}


void MovementHook::WalkInjection(float* joystickXY) {
    auto self = GetSingleton();

    bool shouldWalk;
    if (self->Config_ToggleWalk.get_data()) {
        shouldWalk = self->_walkToggledOn;
    }
    else {
        shouldWalk = GetAsyncKeyState(self->Config_WalkKey.get_vkey_data()) < 0;
    }

    if (shouldWalk) {
        auto x = &joystickXY[0];
        auto y = &joystickXY[1];

        // For some reason the thresholds (at least the deadzone threshold)
        // are applied per-axis, not for overall magnitude.
        if (std::abs(*x) > WALK_SPEED) {
            *x = (*x > 0 ? 1 : -1) * WALK_SPEED;
        }
        if (std::abs(*y) > WALK_SPEED) {
            *y = (*y > 0 ? 1 : -1) * WALK_SPEED;
        }
    }
}

void MovementHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_WalkKey, "ctrl");
    CONFIG_BIND(Config_ToggleWalk, false);

    _hook = dku::Hook::AddCaveHook(
        WalkInjectionSite,
        std::make_pair(0, 6),
        FUNC_INFO(WalkInjection),
        PATCH(
            "\x48\x8d\x4d\x40" // lea rcx,[rbp + 0x40]
        ),
        PATCH(""),
        HookFlag::kRestoreAfterEpilog
    );
}

void MovementHook::Enable() {
    _hook->Enable();
    InputManager::GetSingleton()->register_on_keyDown(on_keyDown);
}

void MovementHook::Disable() {
    _hook->Disable();
    InputManager::GetSingleton()->free_on_keyDown(on_keyDown);
}
