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

static float velX = 0;
static float velY = 0;

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

                if (self->Config_Overhaul.get_data()) {
                    // The alternative game mode changes how the game works to try
                    // to get an analogous experience to what controller players have.

                    auto targetX = (float)p.x;
                    auto targetY = (float)p.y;

                    ClientToReticleCoords(rect, &targetX, &targetY);

                    auto dampingRate = self->Config_Overhaul_Damping.get_data();
                    auto springConstant = self->Config_Overhaul_Frequency.get_data();

                    // We want the reticle movement to reflect a damped harmonic oscillator.
                    // While this is fairly easy to approximate, doing it in a timestep-independent
                    // manner is more complicated.
                    // See https://www.entropy.energy/scholar/node/damped-harmonic-oscillator for math.

                    // The case we're concerned with is the under-damped oscillator. This has
                    // a relatively clean-looking solution, but only if you collapse a bunch of
                    // information into a couple of constants. Those constants can be derived from
                    // our variables, but it would be so complicated that we may as well just use
                    // the more verbose general form (though with imaginary numbers pre-factored out).

                    // Desmos graph: https://www.desmos.com/calculator/fwlgfqevnn

                    // Inputs
                    //     x_0: current position
                    //     x_t: mouse position
                    //     v_0: current velocity
                    //     D  : damping rate
                    //     k  : spring constant
                    //     t  : time step length
                    // Outputs
                    //     x  : new position
                    //     v  : new velocity
                    auto solve = [=](float target, float* reticle, float* vel) {
                        // Let d = x_0 - x_t
                        auto distance = *reticle - target;
                        // Let f = e^(-Dt/2)
                        auto decayFactor = std::exp(-dampingRate * deltaT / 2);
                        // Let w = sqrt(k - (D^2)/4)
                        auto omega = std::sqrt(springConstant - (dampingRate * dampingRate / 4));
                        // Let s = sin(wt)
                        auto sinPart = std::sin(omega * deltaT);
                        // Let c = cos(wt)
                        auto cosPart = std::cos(omega * deltaT);
                        // Let o = dc + ((2v_0 + Dd) / 2w) * s
                        auto oscillationFactor =
                            distance * cosPart + (2 * (*vel) + (dampingRate * distance)) / (2 * omega) * sinPart;
                        
                        // x = fo + x_t
                        *reticle = decayFactor * oscillationFactor + target;
                        // v = dx/dt = f/4w * (v_0(4wc - 2Ds) - (ds(D^2 + 4w^2)))
                        *vel = (decayFactor / (4 * omega)) * (
                            (*vel * (4 * omega * cosPart - (2 * dampingRate * sinPart)))
                            - (distance * sinPart * (dampingRate * dampingRate + (4 * omega * omega)))
                        );
                    };

                    solve(targetX, reticleX, &self->velX);
                    solve(targetY, reticleY, &self->velY);
                }
                else {
                    // The normal game mode reflects how the game normally works,
                    // but is somewhat awkward to actually play on mouse.
                    double height = rect.bottom - rect.top;
                    float normalizedWindowSize = height / 720.;
                    float normalizedSensitivity = normalizedWindowSize * self->Config_MouseSensitivity.get_data();

                    *reticleX += (double)self->cumulativeXDelta * normalizedSensitivity;
                    *reticleY += (double)self->cumulativeYDelta * normalizedSensitivity;

                    // Need to make sure returnSpeed is restored after use in case someone disables the hook.
                    float oldReturnSpeed = *returnSpeed;

                    *returnSpeed = self->Config_ReturnSpeed.get_data();
                    UpdateReticlePosition(obj, deltaT);
                    *returnSpeed = oldReturnSpeed;
                }
            }
        }
        else {
            UpdateReticlePosition(obj, deltaT);
        }
    }
    
    self->cumulativeXDelta = 0;
    self->cumulativeYDelta = 0;
}

void DartsHook::ResetState() {
    cumulativeXDelta = 0;
    cumulativeYDelta = 0;
    velX = 0;
    velY = 0;
}

void DartsHook::ResetReticleInjection(uintptr_t obj) {
    auto self = GetSingleton();

    self->ResetState();

    if (self->Config_Overhaul.get_data()) {
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
    auto self = GetSingleton();
    self->cumulativeXDelta += xDelta;
    self->cumulativeYDelta += yDelta;
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

    // The overhaul completely changes the darts game to act like a dampened harmonic oscillator
    // where the mouse is the base of the spring and the reticle is the mass on the spring.
    // In my opinion this is a much better experience on mouse than the direct port of the minigame.
    CONFIG_BIND(Config_Overhaul, false);
    // The frequency must be greater than (damping^2 / 4).
    // Larger values make it hard to counter the oscillation while smaller frequencies can be too slow.
    CONFIG_BIND(Config_Overhaul_Frequency, 10);
    // 0 = no damping (not advisable). Large values can make the game too easy.
    CONFIG_BIND(Config_Overhaul_Damping, 0.5);

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
    ResetState();

    _updateReticleHook->Enable();
    _resetReticleHook->Enable();
    InputManager::GetSingleton()->register_on_mouseMove(on_mouseMove);
}

void DartsHook::Disable() {
    _updateReticleHook->Disable();
    _resetReticleHook->Disable();
    InputManager::GetSingleton()->free_on_mouseMove(on_mouseMove);
}
