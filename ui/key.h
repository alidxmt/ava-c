#pragma once
#include "UI.h"
#include "Waveform.h"
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <SDL.h>
#include "daisysp.h"

class Key : public Rect {
public:
    enum SourceType { Sine, Square, Saw, Wavetable };

    SourceType source = Wavetable;

    std::unique_ptr<daisysp::Oscillator> osc;
    std::unique_ptr<daisysp::Oscillator> oscDetuned; // 🔹 for detune
    std::vector<float> wavetable;
    size_t tableSize = 2048;
    double phase = 0.0;
    double phaseInc = 0.0;
    double phaseDetuned = 0.0; // 🔹 for detuned wavetable

    double frequency = 440.0;
    double defaultSampleRate = 48000.0;
    float gain = 0.0f;
    float lastGain = 0.0f; // ✅ track previous gain for diagnostics


    float targetGain = 0.0f;
    float attackTime = 0.22f;
    float releaseTime = 0.35f;
    bool active = false;

    enum EnvState { Idle, Attack, Sustain, Release };
    EnvState envState = Idle;

    // Tremolo
    float tremRateParam  = 0.0f; // 0..1
    float tremDepthParam = 0.0f; // 0..1

    // Detune
    float detuneRangeCents = 0.0f; // ±600 cents
    float detuneAmount = 0.0f;

    Key(float x, float y, float w, float h,
        int circleNum = 0, const std::string& txt = "")
        : Rect(x, y, w, h, 0.0f, circleNum, txt) {
        setOscillator(Sine);
        setFrequency(frequency);
    }

    // 🔹 Fletcher–Munson equal-loudness weighting (ISO 226 approx)

    inline float equalLoudnessWeight(float freqHz) const {
        // Protect against invalid input
        if (freqHz < 20.0f) freqHz = 20.0f;
        if (freqHz > 20000.0f) freqHz = 20000.0f;

        double f2 = freqHz * freqHz;

        // RA(f) from A-weighting standard
        double num = pow(12200.0, 2) * f2 * f2;
        double den = (f2 + pow(20.6, 2))
                * sqrt((f2 + pow(107.7, 2)) * (f2 + pow(737.9, 2)))
                * (f2 + pow(12200.0, 2));
        double Ra = num / den;

        // A-weighting in dB
        double AdB = 20.0 * log10(Ra) + 2.0;

        // Convert back to linear gain, inverted
        // return pow(10.0, -(AdB) / 20.0); //main one
        return pow(10.0, -(AdB * 0.3) / 20.0); // only 30% compensation

    }





    // 🔹 Mix this key into the shared audio buffer
    void addToBuffer(float* buffer, int numFrames, double sampleRate = 48000.0) {
        for (int i = 0; i < numFrames; i++) {
            buffer[i] += process(sampleRate);
        }
    }

    void draw(NVGcontext* vg) override {
        // --- base rectangle fill ---
        Rect::draw(vg);

        // --- active overlay ---
        float maxIntensity = 0.0f;
        for (auto &kv : activeTouches)
            maxIntensity = std::max(maxIntensity, kv.second);
        if (maxIntensity > 0.0f) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, cornerRadius);
            NVGcolor c = hoverColor;
            c.a = maxIntensity;
            nvgFillColor(vg, c);
            nvgFill(vg);
        }

        // --- stroke for visible gap ---
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, cornerRadius);
        nvgStrokeWidth(vg, 2.0f);   // 2px stroke
        nvgStrokeColor(vg, srgbColor(24,26,29)); // background color (adjust as needed)
        nvgStroke(vg);
    }



    void setOscillator(SourceType type) {
        source = type;
        osc.reset();
        oscDetuned.reset();
        if (type == Sine || type == Square || type == Saw) {
            auto o1 = std::make_unique<daisysp::Oscillator>();
            auto o2 = std::make_unique<daisysp::Oscillator>();
            o1->Init(defaultSampleRate);
            o2->Init(defaultSampleRate);
            if (type == Sine) {
                o1->SetWaveform(daisysp::Oscillator::WAVE_SIN);
                o2->SetWaveform(daisysp::Oscillator::WAVE_SIN);
            }
            if (type == Square) {
                o1->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
                o2->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
            }
            if (type == Saw) {
                o1->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
                o2->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
            }
            o1->SetAmp(0.5f);
            o2->SetAmp(0.5f);
            osc = std::move(o1);
            oscDetuned = std::move(o2);
        }
    }

    void setWavetable(const std::vector<float>& table) {
        source = Wavetable;
        osc.reset();
        oscDetuned.reset();
        wavetable = table;
        tableSize = table.size();
        phase = 0.0;
        phaseDetuned = 0.0;
    }

    void setFrequency(double freq) {
        setFrequency(freq, defaultSampleRate);
    }

    void setFrequency(double freq, double sampleRate) {
        frequency = freq;
        phaseInc = (frequency / sampleRate) * (double)tableSize;
        if (osc) osc->SetFreq(frequency);
        if (oscDetuned) oscDetuned->SetFreq(frequency);
    }

    bool handleEvent(const SDL_Event& e, int winW, int winH) override {
        float mx = 0, my = 0;
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            mx = e.tfinger.x * winW;
            my = e.tfinger.y * winH;
        }

        if (e.type == SDL_FINGERDOWN) {
            if (isInside(mx, my)) {
                float intensity = computeIntensity(my);
                detuneAmount = computeDetune(mx);
                activeTouches[e.tfinger.fingerId] = intensity;
                noteOn(intensity);
                return true;
            }
        }
        if (e.type == SDL_FINGERMOTION) {
            if (activeTouches.find(e.tfinger.fingerId) != activeTouches.end()) {
                if (isInside(mx, my)) {
                    float intensity = computeIntensity(my);
                    detuneAmount = computeDetune(mx);
                    activeTouches[e.tfinger.fingerId] = intensity;
                    noteMove(intensity);
                    return true;
                } else {
                    activeTouches.erase(e.tfinger.fingerId);
                    if (activeTouches.empty()) noteOff();
                }
            }
        }
        if (e.type == SDL_FINGERUP) {
            activeTouches.erase(e.tfinger.fingerId);
            if (activeTouches.empty()) noteOff();
            return true;
        }
        return false;
    }

    
float process(double sampleRate = 48000.0) {
    lastGain = gain;

    if (!active) return 0.0f;

    float stepAttack  = 1.0f / (attackTime * sampleRate);
    float stepRelease = 1.0f / (releaseTime * sampleRate);

    switch (envState) {
        case Attack:
            gain += stepAttack;
            if (gain >= targetGain) {
                gain = targetGain;
                envState = Sustain;
            }
            break;
        case Sustain:
            gain += (targetGain - gain) * 0.002f;
            break;
        case Release:
            gain *= 0.9995f;   // ✅ exponential release
            if (gain <= 0.0001f) {
                gain = 0;
                envState = Idle;
                active = false;
            }
            break;
        case Idle:
        default:
            return 0.0f;
    }

    float sample = 0.0f;
    float ratio  = powf(2.0f, detuneAmount / 1200.0f);

    if (osc && oscDetuned) {
        osc->SetFreq(frequency);
        oscDetuned->SetFreq(frequency * ratio);
        sample = 0.5f * (osc->Process() + oscDetuned->Process());
    }
    else if (!wavetable.empty()) {
        size_t idx1 = (size_t)phase % tableSize;
        float s1 = wavetable[idx1];

        phaseDetuned += (frequency * ratio / sampleRate) * tableSize;
        if (phaseDetuned >= (double)tableSize)
            phaseDetuned -= (double)tableSize;
        size_t idx2 = (size_t)phaseDetuned % tableSize;
        float s2 = wavetable[idx2];

        sample = 0.5f * (s1 + s2);

        phase += phaseInc;
        if (phase >= (double)tableSize)
            phase -= (double)tableSize;
    }

    // ✅ Zero-cross release + fallback timer
    if (pendingRelease) {
        pendingSamples++;
        if (fabs(sample) < 0.001f) {
            envState = Release;
            pendingRelease = false;
        } else if (pendingSamples > (int)(0.05 * sampleRate)) { // ~50 ms
            envState = Release;
            pendingRelease = false;
        }
    }

    // Tremolo (leave as is)
    static double tremPhase = 0.0;
    if (tremDepthParam > 0.0f) {
        float tremRateHz = 1.0f + tremRateParam * 7.0f;
        float fingerFactor = 1.0f - targetGain;
        float effectiveDepth = tremDepthParam * fingerFactor;

        tremPhase += (tremRateHz / sampleRate) * 2.0 * M_PI;
        if (tremPhase >= 2.0 * M_PI)
            tremPhase -= 2.0 * M_PI;

        float trem = std::sin(tremPhase);
        float modAmp = 1.0f + effectiveDepth * 0.3f * trem;
        lastRawSample = sample;
        return sample * gain * modAmp * equalLoudnessWeight((float)frequency);
    }

    lastRawSample = sample;
    return sample * gain * equalLoudnessWeight((float)frequency);
}





    bool isInside(float mx, float my) const {
        return (mx >= x && mx <= x + w && my >= y && my <= y + h);
    }
    // --- Hit-test: does this key contain (mx, my)? ---
    bool hit(float mx, float my) const {
        return (mx >= x && mx <= x + w &&
                my >= y && my <= y + h);
    }
    bool isActive() const { return active; }
    float getGain() const { return gain; }
    float getLastGain() const { return lastGain; }
    double getFrequency() const { return frequency; }
    float getLastSample() const { return lastRawSample; }


private:
    std::map<SDL_FingerID, float> activeTouches;
    float lastRawSample = 0.0f;
    bool pendingRelease = false;
    int pendingSamples = 0;   // track how long we've been waiting


    float computeIntensity(float my) {
        float relY = (my - y) / h;
        float d = std::abs(relY - 0.5f) * 2.0f;
        // return std::max(0.0f, 1.0f - d);
        // return std::max(0.0f, 1.0f - powf(d, 0.5f));
        return (expf(-4.0f * d) - expf(-4.0f)) / (1.0f - expf(-4.0f));


    }

    float computeDetune(float mx) {
        float relX = (mx - x) / w;
        float centered = (relX - 0.5f) * 2;
        return centered * detuneRangeCents;
    }

    void noteOn(float relGain) { targetGain = relGain; envState = Attack; active = true; }
    void noteMove(float relGain) { targetGain = relGain; }
    // void noteMove(float relGain) {
    //     // Smooth the jump: move targetGain gradually
    //     float smoothing = 0.1f;  // 0..1, smaller = slower
    //     targetGain += (relGain - targetGain) * smoothing;
    // }

    // void noteOff() { envState = Release; }
    void noteOff() {
        if (source == Wavetable) {
            envState = Release;
            pendingRelease = false;
            pendingSamples = 0;
        } else {
            envState = Sustain;
            pendingRelease = true;
            pendingSamples = 0;   // ✅ reset here too
        }
    }


    

};
