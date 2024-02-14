#include "MapHook.hpp"

#define PATCH(code) dku::Hook::Patch { code, sizeof(code) - 1 }

typedef byte func_MoveReticle(uintptr_t, uint64_t);
typedef byte func_ChangeZoom(uintptr_t, float);

const auto MapControlAddress =
    AsAddress(dku::Hook::search_pattern<
        "56 "
        "49 8d ab 78 fd ff ff "
        "48 81 ec 70 03 00 00"
    >()) - 0x5; // 118d175
const auto MoveReticle = reinterpret_cast<func_MoveReticle*>(
    AsAddress(dku::Hook::search_pattern<
        "48 8b 51 70 "
        "f3 0f 10 44 24 20 "
        "f3 0f 10 4c 24 24 "
        "f3 0f 11 41 38"
    >()) - 0x1e // 13495ce
);
const auto ChangeZoom = reinterpret_cast<func_ChangeZoom*>(
    AsAddress(dku::Hook::search_pattern<
        "f3 0f 11 89 00 01 00 00 "
        "48 8b 89 20 01 00 00 "
        "48 85 c9"
    >()) // 13548c0
);

event_result MapHook::on_scroll(int16_t delta) {
    auto self = GetSingleton();

    self->_scrollTicks += (float)delta / WHEEL_DELTA;
    self->_lastScrollTime = std::chrono::system_clock::now();

    if (std::chrono::system_clock::now() - 34ms < self->_lastMapTime) {
        // Scrolling normally acts like up/down arrows, so we want to block
        // scrolling from reaching the game while we're in the map.
        return Cancel;
    }
    return Continue;
}


void MapHook::SetMapCursorPosition(uintptr_t param1) {
    auto self = GetSingleton();

    self->_lastMapTime = std::chrono::system_clock::now();

    auto reticleX = &Memory::deref<float>(param1, 0x624);
    auto reticleY = &Memory::deref<float>(param1, 0x628);
    auto screenX = &Memory::deref<float>(param1, 0x5e4);
    auto screenY = &Memory::deref<float>(param1, 0x5e0);
    auto _changeZoomArg = Memory::deref<uintptr_t>(param1, 0x768);
    auto zoom = &Memory::deref<float>(_changeZoomArg, 0x100);

    // Allow one frame of tolerance at 30fps
    if (self->_scrollTicks != 0 && std::chrono::system_clock::now() - 34ms < self->_lastScrollTime) {
        auto zoomPower = GetSingleton()->Config_ZoomSensitivity.get_data() / 50.;
        auto newZoom = *zoom * std::powf(1.f + zoomPower, self->_scrollTicks);
        newZoom = std::clamp(newZoom, 0.015f, 0.12f);
        // We can't assign directly to zoom because the visuals won't update properly
        ChangeZoom(_changeZoomArg, newZoom);
    }
    self->_scrollTicks = 0;

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
                float mouseX = (float)p.x / (float)width;
                float mouseY = (float)p.y / (float)height;
                
                // These were measured experimentally
                constexpr float mouseBoundsLeft = 0.31796876;
                constexpr float mouseBoundsRight = 0.9640625;
                constexpr float mouseBoundsTop = 0.16180556;
                constexpr float mouseBoundsBottom = 0.8875;
                constexpr float reticleBoundsLeftRight = 620;
                constexpr float reticleBoundsBottomTop = 390;

                constexpr float mouseBoundsWidth = mouseBoundsRight - mouseBoundsLeft;
                constexpr float mouseBoundsHeight = mouseBoundsBottom - mouseBoundsTop;
                constexpr float reticleBoundsWidth = reticleBoundsLeftRight * 2;
                constexpr float reticleBoundsHeight = reticleBoundsBottomTop * 2;

                float normalizedMouseX = (mouseX - mouseBoundsLeft) / mouseBoundsWidth;
                float normalizedMouseY = (mouseY - mouseBoundsTop) / mouseBoundsHeight;

                // This logic makes it so that your mouse doesn't jump around after you click a marker
                static bool hasMovedMouse = false;
                auto forceRelocateMouse = false;
                if (hasMovedMouse) {
                    if (*reticleX == 0 && *reticleY == 0) {
                        // The cursor has been reset and we should follow it.
                        normalizedMouseX = 0.5;
                        normalizedMouseY = 0.5;
                        hasMovedMouse = false;
                        forceRelocateMouse = true;
                    }
                }
                else {
                    if (normalizedMouseX != 0.5 || normalizedMouseY != 0.5) {
                        hasMovedMouse = true;
                    }
                }

                // clamp mouse to map area
                if (forceRelocateMouse || GetSingleton()->Config_LockMouse.get_data()) {
                    normalizedMouseX = std::clamp(normalizedMouseX, 0.f, 1.f);
                    normalizedMouseY = std::clamp(normalizedMouseY, 0.f, 1.f);
                    p = {
                        .x = (int)((normalizedMouseX * mouseBoundsWidth + mouseBoundsLeft) * width),
                        .y = (int)((normalizedMouseY * mouseBoundsHeight + mouseBoundsTop) * height)
                    };
                    ClientToScreen(hwnd, &p);
                    SetCursorPos(p.x, p.y);
                }

                auto newReticleX = normalizedMouseX * reticleBoundsWidth + -reticleBoundsLeftRight;
                auto newReticleY = normalizedMouseY * reticleBoundsHeight + -reticleBoundsBottomTop;
                auto dx = newReticleX - *reticleX;
                auto dy = newReticleY - *reticleY;

                *reticleX = newReticleX;
                *reticleY = newReticleY;

                if (GetAsyncKeyState(VK_LBUTTON) < 0) {
                    *screenX += dx / *zoom;
                    *screenY -= dy / *zoom;
                }

                // Update reticle visuals (logic extracted straight from the ghidra disassembly)
                auto lVar20 = Memory::deref<uintptr_t>(Memory::deref<uintptr_t>(param1, 0x770), 0x28);
                uint64_t puVar27 = _mm_unpacklo_ps(
                    _mm_add_ss(
                        Memory::deref<__m128>(param1, 0x624),
                        Memory::deref<__m128>(param1, 0x614)
                    ),
                    _mm_add_ss(
                        Memory::deref<__m128>(param1, 0x618),
                        Memory::deref<__m128>(param1, 0x628)
                    )
                ).m128_u64[0];
                MoveReticle(lVar20, puVar27);
            }
        }
    }
}

void MapHook::Prepare() {
    CONFIG_BIND(Config_EnableHook, true);
    CONFIG_BIND(Config_LockMouse, true);
    Settings::GetSingleton()->MainConfig.Bind<0., 10.>(Config_ZoomSensitivity, 5);

    _hook = dku::Hook::AddCaveHook(
        MapControlAddress,
        std::make_pair(0, 5),
        FUNC_INFO(SetMapCursorPosition),
        PATCH(
            // push arguments
            "\x48\x83\xec\x08"      // sub rsp,8
            "\x51"                  // push rcx
            "\x52"                  // push rdx
            "\x41\x50"              // push r8
            "\x41\x51"              // push r9
            // "push" xmm2
            "\x48\x83\xec\x10"      // sub rsp,16
            "\xf3\x0f\x7f\x14\x24"  // movdqu [rsp],xmm2
        ),
        PATCH(
            // "pop" xmm2
            "\xf3\x0f\x6f\x14\x24"  // movdqu xmm2,[rsp]
            "\x48\x83\xc4\x10"      // add rsp,16
            // pop arguments
            "\x41\x59"              // pop r9
            "\x41\x58"              // pop r8
            "\x5a"                  // pop rdx
            "\x59"                  // pop rcx
            "\x48\x83\xc4\x08"      // add rsp,8
        ),
        HookFlag::kRestoreAfterEpilog
    );
}

void MapHook::Enable() {
    _hook->Enable();
    InputManager::GetSingleton()->register_on_scroll(on_scroll);
}

void MapHook::Disable() {
    _hook->Disable();
    InputManager::GetSingleton()->free_on_scroll(on_scroll);
}
