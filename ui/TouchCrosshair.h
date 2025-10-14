#pragma once
#include <SDL.h>
#include <nanovg.h>
#include <array>

// ----------------------------------------------------
// Independent multitouch crosshair visualizer
// For up to N fingers with very subtle, dark-theme lines
// ----------------------------------------------------
class TouchCrosshair {
public:
    static constexpr int MaxFingers = 5;

    struct Finger {
        bool active = false;
        float x = 0.0f;
        float y = 0.0f;
    };

    std::array<Finger, MaxFingers> fingers;

    // --- Dark theme colors: very faint lines (≈5–8% opacity)
    const NVGcolor colors[MaxFingers] = {
        nvgRGBA(200, 90, 100, 12),   // soft red
        nvgRGBA(100, 180, 220, 12),  // cyan-blue
        nvgRGBA(180, 160, 100, 12),  // gold
        nvgRGBA(150, 100, 200, 12),  // violet
        nvgRGBA(110, 180, 120, 12)   // green
    };

    // -----------------------------------
    // Handle SDL touch input
    // -----------------------------------
    void handleEvent(const SDL_Event& e, int winW, int winH) {
        if (!(e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP))
            return;

        int id = static_cast<int>(e.tfinger.fingerId) % MaxFingers;

        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            fingers[id].active = true;
            fingers[id].x = e.tfinger.x * winW;
            fingers[id].y = e.tfinger.y * winH;
        } else if (e.type == SDL_FINGERUP) {
            fingers[id].active = false;
        }
    }

    // -----------------------------------
    // Draw all active crosshairs (only lines)
    // -----------------------------------
    void draw(NVGcontext* vg, int winW, int winH) {
        nvgSave(vg);

        for (int i = 0; i < MaxFingers; ++i) {
            const auto& f = fingers[i];
            if (!f.active) continue;

            // thin, subtle crosshair lines
            nvgBeginPath(vg);
            nvgStrokeColor(vg, colors[i]);
            nvgStrokeWidth(vg, 0.8f);

            // vertical line
            nvgMoveTo(vg, f.x, 0);
            nvgLineTo(vg, f.x, winH);

            // horizontal line
            nvgMoveTo(vg, 0, f.y);
            nvgLineTo(vg, winW, f.y);

            nvgStroke(vg);
        }

        nvgRestore(vg);
    }

    void clear() {
        for (auto& f : fingers) f.active = false;
    }
};
