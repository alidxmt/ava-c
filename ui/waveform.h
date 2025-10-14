#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <functional>
#include <nanovg.h>
#include <SDL.h>
#include "UI.h"           // for Widget + srgbColor
#include "WaveSchema.h"   // harmonic extrapolation engine

#include "daisysp.h"
using namespace daisysp;


// ------------------- Data Structures -------------------
struct HarmonicSpec {
    std::vector<float> amps;
    std::vector<float> phases;
};

struct WaveformInfo {
    std::string name;
    std::function<std::vector<float>()> generator;
};


// ------------------- Waveform Class -------------------
class Waveform {
public:

    // ===== Utility Builders =====
    static std::vector<Harmonic> toHarmonics(const HarmonicSpec& spec) {
        std::vector<Harmonic> v;
        size_t N = std::min(spec.amps.size(), spec.phases.size());
        v.reserve(N);
        for (size_t i = 0; i < N; ++i) v.push_back({spec.amps[i], spec.phases[i]});
        return v;
    }

    static std::vector<float> buildWithSchema(const HarmonicSpec& spec,
                                              double f0,
                                              double sampleRate = 48000.0,
                                              double bandwidth  = 12000.0,
                                              size_t tableSize  = 2048)
    {
        WaveSchema ws(toHarmonics(spec), f0, sampleRate, bandwidth);
        return ws.buildTable(tableSize);
    }

    static std::vector<float> buildLimited(const HarmonicSpec& spec,
                                           int maxHarmonics,
                                           size_t tableSize = 2048)
    {
        std::vector<float> out(tableSize, 0.0f);
        int N = std::min((int)spec.amps.size(), maxHarmonics);

        for (int h = 1; h <= N; h++) {
            float amp = spec.amps[h-1];
            float phi = spec.phases[h-1];
            for (size_t n = 0; n < tableSize; n++) {
                float t = (2.0f * M_PI * h * n) / tableSize;
                out[n] += amp * sinf(t + phi);
            }
        }

        // normalize
        float maxVal = 0.0f;
        for (auto v : out) maxVal = std::max(maxVal, fabsf(v));
        if (maxVal > 0.0f) for (auto &v : out) v /= maxVal;

        return out;
    }

    // ===== Golden Wave =====
    static HarmonicSpec GoldenSpec() {
        HarmonicSpec spec;
        const float phi = 1.6180339887f;
        const int N = 16; // first 16 harmonics, enough for richness

        for (int k = 1; k <= N; k++) {
            float amp = powf(1.0f / phi, (float)(k-1));  // golden ratio decay
            spec.amps.push_back(amp);
            spec.phases.push_back((k % 2 == 0) ? 0.0f : 0.2f); // tiny offset
        }
        return spec;
    }

    static std::vector<float> GoldenWave(size_t tableSize = 2048) {
        return buildLimited(GoldenSpec(), (int)GoldenSpec().amps.size(), tableSize);
    }

    // ===== General builder for real/imag harmonics =====
    static std::vector<float> buildTable(const std::vector<float>& real,
                                         const std::vector<float>& imag,
                                         size_t tableSize)
    {
        std::vector<float> out(tableSize,0.0f);
        size_t harmonics = std::min(real.size(), imag.size());
        for (size_t i = 0; i < tableSize; i++) {
            double t = (2.0 * M_PI * i) / tableSize;
            double v = 0.0;
            for (size_t k = 0; k < harmonics; k++) {
                v += real[k] * std::cos(k * t) + imag[k] * std::sin(k * t);
            }
            out[i] = float(v);
        }
        return out;
    }

    // ===== Euler-type Wavetables =====
    static std::vector<float> Euler(size_t tableSize = 2048, size_t N = 64) {
        std::vector<float> r(N+1,0), i(N+1,0);
        for (size_t k=1;k<=N;k++) r[k]=std::exp(-float(k)/3.0f);
        return buildTable(r,i,tableSize);
    }

    static std::vector<float> Eulerplus(size_t tableSize = 2048, size_t N = 64, float targetGain=1.0f) {
        std::vector<float> r(N+1,0), i(N+1,0);
        float decayBase=3.0f; 
        float dynDecay=decayBase+3.0f*targetGain;
        for(size_t k=1;k<=N;k++){
            float amp=std::exp(-float(k)/dynDecay)/k;
            float frac=float(k)/float(M_PI)-std::floor(float(k)/float(M_PI));
            float phi=2.0f*float(M_PI)*frac;
            r[k]=amp*std::cos(phi); 
            i[k]=amp*std::sin(phi);
        }
        normalize(r,i);
        return buildTable(r,i,tableSize);
    }

    // ===== BrighterSine =====
    static HarmonicSpec BrighterSineSpec() {
        HarmonicSpec spec;
        spec.amps   = {1.0f, 0.1f, 0.05f, 0.02f, 0.009f, 0.03f, 0.011f, 0.009f, 0.004f, 0.0013f};
        spec.phases = {0,0,0,0,0,0,0,0,0,0};
        return spec;
    }

    static std::vector<float> BrighterSine(size_t tableSize = 2048) {
        return buildLimited(BrighterSineSpec(), (int)BrighterSineSpec().amps.size(), tableSize);
    }

    // ===== Dod =====
    // ===== Dod as Real/Imag Wave =====
    static HarmonicSpec DodSpec() {
        HarmonicSpec spec;
        // spec.amps = {1.000f, 0.050f, 0.017f, 0.009f, 0.002f, 0.001f,
        //             0.00005f, 0.00001f, 0.00015f, 0.00007f, 0.0005f};
        spec.amps = {1.000f, 0.4f, 0.88f, 0.26f, 0.032f, 0.016f,
                    0.004f, 0.001f, 0.0038f, 0.00028f, 0.0002f};
        spec.phases = {-2.05f, -1.00f, -2.89f, 0.60f, 2.05f, -0.12f,
                    -0.75f, -1.77f, 0.72f, -2.16f, -0.78f};
        return spec;
    }

    static std::vector<float> GoldenDodWave(size_t tableSize = 2048) {
        HarmonicSpec spec = DodSpec();

        // Convert Dod amps/phases to real/imag
        std::vector<float> real, imag;
        real.reserve(spec.amps.size());
        imag.reserve(spec.amps.size());
        for (size_t k = 0; k < spec.amps.size(); k++) {
            float r = spec.amps[k] * cosf(spec.phases[k]);
            float i = spec.amps[k] * sinf(spec.phases[k]);
            real.push_back(r);
            imag.push_back(i);
        }

        // Build raw Dod-based wave
        std::vector<float> raw = buildTable(real, imag, tableSize);

        // === Normalize Dod loudness to GoldenWave loudness (RMS + Peak weighted + perceptual boost) ===
        std::vector<float> golden = GoldenWave(tableSize);

        auto rms = [](const std::vector<float>& v) {
            double sum = 0.0;
            for (float x : v) sum += x * x;
            return std::sqrt(sum / v.size());
        };

        auto peak = [](const std::vector<float>& v) {
            float maxVal = 0.0f;
            for (float x : v) {
                float ax = std::fabs(x);
                if (ax > maxVal) maxVal = ax;
            }
            return maxVal;
        };

        float rmsDod     = rms(raw);
        float rmsGolden  = rms(golden);
        float peakDod    = peak(raw);
        float peakGolden = peak(golden);

        float rmsGain  = (rmsGolden  > 0.0f && rmsDod  > 0.0f) ? (rmsGolden  / rmsDod)  : 1.0f;
        float peakGain = (peakGolden > 0.0f && peakDod > 0.0f) ? (peakGolden / peakDod) : 1.0f;

        // Weighted normalization + perceptual boost
        float gain = 0.5f * (rmsGain + peakGain) * 3.5f;

        for (auto& x : raw) x *= gain;

        return raw;
    }


    // ===== Violin =====
    static std::vector<float> Violin(size_t tableSize = 2048) {
        std::vector<float> out(tableSize, 0.0f);

        struct HarmDef { float amp; int type; }; // 0 = sine, 1 = cos
        HarmDef harmonics[] = {
            {0.490f, 0}, {0.995f, 0}, {0.940f, 1}, {0.425f, 0}, {0.480f, 1},
            {0.000f, 0}, {0.365f, 1}, {0.040f, 0}, {0.085f, 1}, {0.000f, 0}, {0.090f, 1}
        };

        constexpr size_t N = sizeof(harmonics) / sizeof(HarmDef);

        for (size_t i = 0; i < tableSize; i++) {
            float t = (2.0f * M_PI * i) / tableSize;
            float v = 0.0f;
            for (size_t k = 0; k < N; k++) {
                if (harmonics[k].amp == 0.0f) continue;
                if (harmonics[k].type == 0) v += harmonics[k].amp * std::sin((k+1) * t);
                else                        v += harmonics[k].amp * std::cos((k+1) * t);
            }
            out[i] = v;
        }
        return out;
    }

    // ===== Available Waveforms =====
    static std::vector<WaveformInfo> available() {
        return {
            {"Sine", [](){ return std::vector<float>(); }},
            {"GoldenWave", [](){ return GoldenWave(); }},
            {"BrighterSine", [](){ return BrighterSine(); }},
            {"Dod", [](){ return GoldenDodWave(); }},
            {"DodExtended", [](){ return GoldenDodWave(); }},
            {"Violin", [](){ return Violin(); }},
            {"Euler", [](){ return Euler(); }},
            {"Eulerplus", [](){ return Eulerplus(); }},
            {"Custom", [](){ return std::vector<float>(); }}
        };
    }

private:
    static void normalize(std::vector<float>& a, std::vector<float>& b) {
        double e = 0.0;
        for (size_t k = 1; k < a.size(); ++k) e += a[k]*a[k] + b[k]*b[k];
        if (e > 0.0) {
            float g = 1.0f / std::sqrt(float(e));
            for (size_t k = 1; k < a.size(); ++k) { a[k]*=g; b[k]*=g; }
        }
    }
};



// -------------------------
// Waveform Selector
// -------------------------
class WaveformSelector : public Widget {
public:
    int currentIndex = 0;
    std::vector<WaveformInfo> options;
    std::function<void(int)> onSelect;

    WaveformSelector(float x_, float y_, float w_, float h_)
        : Widget(x_, y_, w_, h_) {
        options = Waveform::available();
    }

    void draw(NVGcontext* vg) override {
        if (options.empty()) return;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 4.0f);
        nvgFillColor(vg, srgbColor(24,26,29));
        nvgFill(vg);

        nvgStrokeColor(vg, srgbColor(187,125,128));
        nvgStrokeWidth(vg, 0.8f);
        nvgStroke(vg);

        nvgFontFace(vg, "ui");
        nvgFontSize(vg, std::min(h * 0.5f, 18.0f));
        nvgFillColor(vg, srgbColor(217,211,215));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        const std::string& label = options[currentIndex].name;
        nvgText(vg, x + w * 0.5f, y + h * 0.5f, label.c_str(), nullptr);
    }

    bool handleEvent(const SDL_Event& e, int winW, int winH) {
        if (e.type == SDL_FINGERDOWN) {
            float mx = e.tfinger.x * winW;
            float my = e.tfinger.y * winH;
            if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
                currentIndex = (currentIndex + 1) % (int)options.size();
                if (onSelect) onSelect(currentIndex);
                return true;
            }
        }
        return false;
    }

    WaveformInfo selected() const {
        if (options.empty()) return {"None", [](){ return std::vector<float>(); }};
        return options[currentIndex];
    }
};
