#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------
// Simple biquad filter (can do highpass, lowpass, etc.)
// ---------------------------------------------------------

struct Biquad {
    float a0 = 1, a1 = 0, a2 = 0, b1 = 0, b2 = 0;
    float z1 = 0, z2 = 0;

    void setupHighpass(float sampleRate, float cutoffHz, float Q = 0.707f) {
        float w0 = 2.0f * M_PI * cutoffHz / sampleRate;
        float cosw0 = cosf(w0), sinw0 = sinf(w0);
        float alpha = sinw0 / (2.0f * Q);
        float b0n =  (1 + cosw0) / 2.0f;
        float b1n = -(1 + cosw0);
        float b2n =  (1 + cosw0) / 2.0f;
        float a0n = 1 + alpha;
        float a1n = -2 * cosw0;
        float a2n = 1 - alpha;

        a0 = b0n / a0n; a1 = b1n / a0n; a2 = b2n / a0n;
        b1 = a1n / a0n; b2 = a2n / a0n;
        z1 = z2 = 0;
    }

    // 🔹 NEW: 12 dB/oct low-pass
    void setupLowpass(float sampleRate, float cutoffHz, float Q = 0.707f) {
        float w0 = 2.0f * M_PI * cutoffHz / sampleRate;
        float cosw0 = cosf(w0), sinw0 = sinf(w0);
        float alpha = sinw0 / (2.0f * Q);
        float b0n = (1 - cosw0) / 2.0f;
        float b1n =  1 - cosw0;
        float b2n = (1 - cosw0) / 2.0f;
        float a0n =  1 + alpha;
        float a1n = -2 * cosw0;
        float a2n =  1 - alpha;

        a0 = b0n / a0n; a1 = b1n / a0n; a2 = b2n / a0n;
        b1 = a1n / a0n; b2 = a2n / a0n;
        z1 = z2 = 0;
    }

    inline float process(float x) {
        float y = a0 * x + a1 * z1 + a2 * z2 - b1 * z1 - b2 * z2;
        z2 = z1;
        z1 = y;
        return y;
    }
};



// ---------------------------------------------------------
// Simple limiter (hard ceiling)
// ---------------------------------------------------------
struct SimpleLimiter {
    float threshold = 0.9f; // clip limit
    inline float process(float x) {
        float t = threshold;
        if (x > t) return t + (1.0f - expf(-(x - t)));
        if (x < -t) return -t - (1.0f - expf(-(-x - t)));
        return x;
    }

};

// ---------------------------------------------------------
// AudioBus: global FX chain
// ---------------------------------------------------------
class AudioBus {
public:
    AudioBus(float sampleRate = 48000.0f)
        : sampleRate(sampleRate) {
        hpFilter.setupHighpass(sampleRate, 40.0f);     // DC / rumble
        // 🔹 Default: gentle 24 dB/oct @ 6 kHz (two cascaded biquads)
        lpFilter1.setupLowpass(sampleRate, lowpassHz, lowpassQ);
        lpFilter2.setupLowpass(sampleRate, lowpassHz, lowpassQ);
    }

    void setMasterGain(float g) { masterGain = g; }
    void setLimiterThreshold(float t) { limiter.threshold = t; }

    // 🔹 NEW: control LPF at runtime
    void setLowpassHz(float hz) {
        lowpassHz = hz;
        lpFilter1.setupLowpass(sampleRate, lowpassHz, lowpassQ);
        lpFilter2.setupLowpass(sampleRate, lowpassHz, lowpassQ);
    }
    void setLowpassQ(float q) {
        lowpassQ = q;
        lpFilter1.setupLowpass(sampleRate, lowpassHz, lowpassQ);
        lpFilter2.setupLowpass(sampleRate, lowpassHz, lowpassQ);
    }
    void setLowpassEnabled(bool e) { lowpassEnabled = e; }

    void process(float* buffer, int numFrames) {
        float peak = 0.0f;

        for (int i = 0; i < numFrames; i++) {
            float x = buffer[i];

            // 1) highpass (clean lows)
            x = hpFilter.process(x);

            // 2) 🔹 lowpass (tame metallic highs) – BEFORE limiter
            if (lowpassEnabled) {
                x = lpFilter1.process(x); // 12 dB/oct
                x = lpFilter2.process(x); // +12 dB/oct = 24 dB/oct
            }

            // 3) master gain
            x *= masterGain;

            // 4) limiter last
            x = limiter.process(x);

            buffer[i] = x;
            peak = std::max(peak, fabsf(x));
        }

        // Optional auto-normalize
        if (peak > 0.95f) {
            float norm = 0.95f / peak;
            for (int i = 0; i < numFrames; i++) buffer[i] *= norm;
        }
    }

private:
    float sampleRate;
    float masterGain = 0.8f;

    Biquad hpFilter;

    // 🔹 NEW:
    bool  lowpassEnabled = true;
    float lowpassHz      = 6000.0f;   // start safe at 6 kHz
    float lowpassQ       = 0.707f;    // Butterworth-ish, no ringing
    Biquad lpFilter1;                 // cascade for 24 dB/oct
    Biquad lpFilter2;

    SimpleLimiter limiter;
};
