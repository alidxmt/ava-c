#pragma once
#include <vector>
#include <cmath>
#include <nanovg.h>
#include "UI.h"   // ✅ srgbColor is here


// --- Minimal DFT (slow but fine for 2048 samples offline) ---
inline std::vector<float> computeSpectrum(const std::vector<float>& samples) {
    size_t N = samples.size();
    if (N == 0) return {};
    std::vector<float> mag(N/2);

    for (size_t k = 0; k < N/2; k++) {
        double re = 0.0, im = 0.0;
        for (size_t n = 0; n < N; n++) {
            double phase = -2.0 * M_PI * k * n / double(N);
            re += samples[n] * cos(phase);
            im += samples[n] * sin(phase);
        }
        mag[k] = std::sqrt(re*re + im*im) / double(N);
    }
    return mag;
}

// --- Draw spectrum with NanoVG ---
inline void drawSpectrum(NVGcontext* vg, float x, float y, float w, float h,
                         const std::vector<float>& spectrum) {
    if (spectrum.empty()) return;
    size_t N = spectrum.size();
    float barW = w / float(N);

    nvgBeginPath(vg);
    for (size_t i = 0; i < N; i++) {
        float val = spectrum[i];
        float barH = val * h * 20.0f; // scale boost
        if (barH > h) barH = h;
        nvgRect(vg, x + i*barW, y + h - barH, barW, barH);
    }
    nvgFillColor(vg, srgbColor(187,125,128));
    nvgFill(vg);
}
