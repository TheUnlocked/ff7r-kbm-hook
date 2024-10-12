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
    >()) + 0x5; // ea867e


static int cumulativeXDelta = 0;
static int cumulativeYDelta = 0;

// These values were derived experimentally
constexpr float _CTR_scaleFactor = 1.5;
constexpr float _CTR_offsetX = 7.5;
constexpr float _CTR_offsetY = 3.5;

void ClientToReticleCoords(RECT &windowRect, float* x, float* y) {
    double width = windowRect.right - windowRect.left;
    double height = windowRect.bottom - windowRect.top;

    *x = (*x - (width / 2.0) + _CTR_offsetX) * _CTR_scaleFactor;
    *y = (*y - (height / 2.0) + _CTR_offsetY) * _CTR_scaleFactor;
}

void ReticleToClientCoords(RECT &windowRect, float* x, float* y) {
    double width = windowRect.right - windowRect.left;
    double height = windowRect.bottom - windowRect.top;

    *x = (*x / _CTR_scaleFactor) + (width / 2.0) - _CTR_offsetX;
    *y = (*y / _CTR_scaleFactor) + (height / 2.0) - _CTR_offsetY;
}

void DartsHook::UpdateReticlePositionIntercept(uintptr_t obj, float deltaT) {
    auto self = GetSingleton();

    auto returnSpeed = &Memory::deref<float>(obj, 0x8);
    auto reticleX = &Memory::deref<float>(obj, 0x10);
    auto reticleY = &Memory::deref<float>(obj, 0x14);
    auto reticleOriginX = &Memory::deref<float>(obj, 0x18);
    auto reticleOriginY = &Memory::deref<float>(obj, 0x1c);

    POINT p;
    if (GetCursorPos(&p)) {
        auto hwnd = GetForegroundWindow();
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == DllState::processId) {
            RECT rect;
            if (ScreenToClient(hwnd, &p) && GetWindowRect(hwnd, &rect)) {

                if (self->Config_ZExperiment_Alternative.get_data()) {
                    // The alternative game mode changes how the game works to try
                    // to get an analogous experience to what controller players have.

                    auto simulatedOriginX = (float)p.x;
                    auto simulatedOriginY = (float)p.y;

                    ClientToReticleCoords(rect, &simulatedOriginX, &simulatedOriginY);

                    *reticleX += self->Config_ReturnSpeed.get_data() * deltaT * (simulatedOriginX - *reticleX);
                    *reticleY += self->Config_ReturnSpeed.get_data() * deltaT * (simulatedOriginY - *reticleY);
                }
                else {
                    // The normal game mode reflects how the game normally works,
                    // but is somewhat awkward to actually play on mouse.
                    double height = rect.bottom - rect.top;
                    float normalizedWindowSize = height / 720.;
                    float normalizedSensitivity = normalizedWindowSize * self->Config_MouseSensitivity.get_data();

                    *reticleX += (double)cumulativeXDelta * normalizedSensitivity;
                    *reticleY += (double)cumulativeYDelta * normalizedSensitivity;

                    // Need to make sure returnSpeed is restored after use in case someone disables the hook.
                    float oldReturnSpeed = *returnSpeed;

                    *returnSpeed = self->Config_ReturnSpeed.get_data();
                    UpdateReticlePosition(obj, deltaT);
                    *returnSpeed = oldReturnSpeed;
                }
            }
        }
    }
    
    cumulativeXDelta = 0;
    cumulativeYDelta = 0;
}

void DartsHook::ResetReticleInjection(uintptr_t obj) {
    cumulativeXDelta = 0;
    cumulativeYDelta = 0;

    if (GetSingleton()->Config_ZExperiment_Alternative.get_data()) {
        RECT windowRect;
        GetWindowRect(DllState::window, &windowRect);

        float x = Memory::deref<float>(obj, 0x58);
        float y = Memory::deref<float>(obj, 0x5c);
        ReticleToClientCoords(windowRect, &x, &y);

        POINT p = { .x = (int)x, .y = (int)y };
        ClientToScreen(DllState::window, &p);
        SetCursorPos(p.x, p.y);
    }
}

event_result DartsHook::on_mouseMove(int32_t xDelta, int32_t yDelta) {
    cumulativeXDelta += xDelta;
    cumulativeYDelta += yDelta;
    return Continue;
}

void DartsHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_MouseSensitivity, 1.0);
    // The original "return speed" value in-game is 1.8, but that's intended for controller where
    // keeping the reticle in the same position position requires staying still, as opposed to on
    // mouse where it requires continuing to move but at a constant rate.
    // Particularly on less-high resolution input devices and smaller mousepads, being able to
    // configure this is critical to make sure the minigame isn't stupidly difficult.
    CONFIG_BIND(Config_ReturnSpeed, 1.8);
    // The alternative game mode
    CONFIG_BIND(Config_ZExperiment_Alternative, false);

    _updateReticleHook = dku::Hook::AddRelHook<5, true>(
        UpdateReticlePositionCallerAddress,
        AsAddress(&UpdateReticlePositionIntercept)
    );

    _resetReticleHook = dku::Hook::AddCaveHook(
        ResetReticleInjectionSite,
        std::make_pair(0, 5),
        FUNC_INFO(ResetReticleInjection),
        PATCH(
            "\x48\x89\xd9" // mov rcx,rbx
        ),
        PATCH(""),
        HookFlag::kRestoreAfterEpilog
    );
}

void DartsHook::Enable() {
    firstDartsTick = true;
    _updateReticleHook->Enable();
    _resetReticleHook->Enable();
    InputManager::GetSingleton()->register_on_mouseMove(on_mouseMove);
}

void DartsHook::Disable() {
    _updateReticleHook->Disable();
    _resetReticleHook->Disable();
    InputManager::GetSingleton()->free_on_mouseMove(on_mouseMove);
}
