#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <random>

// ------------------- WaveSchema -------------------
struct Harmonic {
    float amp;
    float phase;
};

class WaveSchema {
public:
    // Input: list of measured harmonics (1..N), base frequency, sample rate, target bandwidth
    WaveSchema(const std::vector<Harmonic>& baseHarmonics,
               double baseFreq,
               double sampleRate = 48000.0,
               double targetBandwidth = 12000.0)
        : f0(baseFreq), Fs(sampleRate), B(targetBandwidth)
    {
        harmonics = extendHarmonics(baseHarmonics);
    }

    // Build wavetable from schema
    std::vector<float> buildTable(size_t tableSize = 2048) const {
        std::vector<float> out(tableSize, 0.0f);
        for (size_t n = 0; n < tableSize; n++) {
            double t = (2.0 * M_PI * n) / tableSize;
            double v = 0.0;
            for (size_t k = 0; k < harmonics.size(); k++) {
                int h = k + 1;
                v += harmonics[k].amp * std::sin(h * t + harmonics[k].phase);
            }
            out[n] = (float)v;
        }
        // normalize
        float maxAmp = 0.0f;
        for (auto v : out) maxAmp = std::max(maxAmp, std::fabs(v));
        if (maxAmp > 0.0f)
            for (auto &v : out) v /= maxAmp;
        return out;
    }

    // Access to the harmonic list
    const std::vector<Harmonic>& getHarmonics() const { return harmonics; }

private:
    double f0, Fs, B;
    std::vector<Harmonic> harmonics;

    // Extrapolate harmonics using a simple rolloff model
    std::vector<Harmonic> extendHarmonics(const std::vector<Harmonic>& base) {
        // --- how many we need? ---
        int HmaxNyquist = (int)std::floor((Fs * 0.5) / f0);
        int Hneeded     = (int)std::floor(B / f0);
        int Htarget     = std::min(HmaxNyquist, Hneeded);

        std::vector<Harmonic> extended = base;
        if (extended.empty()) return extended;

        int K = (int)base.size();

        // Fit simple power-law rolloff: amp ~ 1/k^p
        double p = 1.0;
        if (K >= 2) {
            // crude slope from first and last harmonic
            double a1 = std::log(std::max(1e-9f, base[0].amp));
            double a2 = std::log(std::max(1e-9f, base[K-1].amp));
            p = (a1 - a2) / std::log((double)K);
        }

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> randPhase(-M_PI, M_PI);

        // Extend beyond measured
        for (int k = K+1; k <= Htarget; k++) {
            float amp = base[0].amp * std::pow(1.0 / k, p);
            float phase = randPhase(rng); // randomize high harmonics
            extended.push_back({amp, phase});
        }

        return extended;
    }
};


