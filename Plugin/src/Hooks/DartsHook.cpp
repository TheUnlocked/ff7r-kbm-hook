#include "DartsHook.hpp"

#define PATCH(code) dku::Hook::Patch { code, sizeof(code) - 1 }

typedef byte func_UpdateReticlePosition(uintptr_t, float deltaT);

const auto UpdateReticlePositionCallerAddress =
    AsAddress(dku::Hook::search_pattern<
        "f3 0f 10 7b 58 "
        "48 8b cb"
    >()) - 0x5; // ea8909

const auto UpdateReticlePosition = reinterpret_cast<func_UpdateReticlePosition*>(
    AsAddress(dku::Hook::search_pattern<
        "0f 29 74 24 20 "
        "0f 28 f1 "
        "ff 10"
    >()) - 0xc // dda9f0
);

const auto ResetReticleInjectionSite =
    AsAddress(dku::Hook::search_pattern<
        "f3 0f 11 53 5c "
        "48 8b 5c 24 60"
    >()) + 0xa; // ea8674


void DartsHook::UpdateReticlePositionIntercept(uintptr_t obj, float deltaT) {
    auto self = GetSingleton();

    // auto returnSpeed = &Memory::deref<float>(obj, 0x8);
    auto reticleX = &Memory::deref<float>(obj, 0x10);
    auto reticleY = &Memory::deref<float>(obj, 0x14);

    POINT p;
    if (GetCursorPos(&p)) {
        auto hwnd = GetForegroundWindow();
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == DllState::currentProcessId) {
            RECT rect;
            if (ScreenToClient(hwnd, &p) && GetWindowRect(hwnd, &rect)) {
                auto width = rect.right - rect.left;
                auto height = rect.bottom - rect.top;

                if (self->firstDartsTick) {
                    self->firstDartsTick = false;
                }
                else {
                    auto dx = p.x - (width / 2);
                    auto dy = p.y - (height / 2);

                    float normalizedWindowSize = ((double)height / 720.);
                    float normalizedSensitivity = normalizedWindowSize * self->Config_MouseSensitivity.get_data();

                    *reticleX += (double)dx * normalizedSensitivity;
                    *reticleY += (double)dy * normalizedSensitivity;
                }

                // Move cursor to the middle of the window so that it never hits the edges of the screen
                p = { .x = width / 2, .y = height / 2 };
                ClientToScreen(hwnd, &p);
                SetCursorPos(p.x, p.y);
            }
        }
    }

    // // Need to make sure returnSpeed is restored after use in case someone disables the hook.
    // float oldReturnSpeed = *returnSpeed;

    // *returnSpeed = self->Config_ReturnSpeed.get_data();
    UpdateReticlePosition(obj, deltaT);
    // *returnSpeed = oldReturnSpeed;
}

void DartsHook::ResetReticleInjection() {
    // It might seem better to reset the mouse position here, but for some mysterious reason,
    // attempting to do so also moves the _reticle_ to the center of the window, not just the mouse.
    // Maybe there's something in this code that would do that, but I can't find it,
    // so my only thought is that maybe FF7R is actually briefly using the mouse position
    // to store the origin location, and overwriting it here breaks that.
    // Regardless, using this state variable works fine.
    GetSingleton()->firstDartsTick = true;
}

void DartsHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_MouseSensitivity, 1.0);
    // CONFIG_BIND(Config_ReturnSpeed, 1.8);

    _updateReticleHook = dku::Hook::AddRelHook<5, true>(
        UpdateReticlePositionCallerAddress,
        AsAddress(&DartsHook::UpdateReticlePositionIntercept)
    );

    _resetReticleHook = dku::Hook::AddCaveHook(
        ResetReticleInjectionSite,
        std::make_pair(0, 5),
        FUNC_INFO(ResetReticleInjection),
        PATCH(""),
        PATCH(""),
        HookFlag::kRestoreAfterEpilog
    );
}

void DartsHook::Enable() {
    firstDartsTick = true;
    _updateReticleHook->Enable();
    _resetReticleHook->Enable();
}

void DartsHook::Disable() {
    _updateReticleHook->Disable();
    _resetReticleHook->Disable();
}
