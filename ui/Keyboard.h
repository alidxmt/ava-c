#pragma once
#include "Key.h"
#include <vector>
#include <map>
#include <string>
#include <SDL.h>
#include <nanovg.h>
#include <cmath>
#include "AudioBus.h"
#include "WaveSchema.h"
#include "Waveform.h"
#define NOMINMAX
#include <algorithm>

class Keyboard {
public:
    Keyboard(int numKeys,
             float baseFrequency,
             const std::vector<double>& ratios,
             const std::vector<std::string>& labels,
             Key::SourceType defaultWaveform,
             float keyWidth, float keyHeight, float gap,
             float startX, float yPos)
        : numKeys(numKeys),
          baseFrequency(baseFrequency),
          ratios(ratios),
          labels(labels),
          defaultWaveform(defaultWaveform),
          keyWidth(keyWidth),
          keyHeight(keyHeight),
          gap(gap),
          startX(startX),
          yPos(yPos)
    {
        buildKeys();
    }
    bool calligraphyEnabled = false;   // 🔹 Toggle Calligraphy mode

    void draw(NVGcontext* vg) {
        for (auto& k : keys) k.draw(vg);
    }

    // --- AudioBus integration ---
    void setMasterGain(float g) { bus.setMasterGain(g); }
    void setLimiterThreshold(float t) { bus.setLimiterThreshold(t); }
    
    // --- Lowpass passthroughs ---
    void setLowpassHz(float hz)        { bus.setLowpassHz(hz); }
    void setLowpassQ(float q)          { bus.setLowpassQ(q); }
    void setLowpassEnabled(bool e)     { bus.setLowpassEnabled(e); }

    void process(float* buffer, int numFrames) {
        for (auto& k : keys) k.addToBuffer(buffer, numFrames);
        bus.process(buffer, numFrames);
    }

    bool handleEvent(const SDL_Event& e, int winW, int winH) {
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION) {
            float mx = e.tfinger.x * winW;
            float my = e.tfinger.y * winH;

            if (e.type == SDL_FINGERDOWN) {
                for (int i = 0; i < (int)keys.size(); i++) {
                    if (keys[i].isInside(mx, my)) {
                        keys[i].handleEvent(e, winW, winH);
                        fingerToKey[e.tfinger.fingerId] = i;
                        return true;
                    }
                }
            } else if (e.type == SDL_FINGERMOTION) {
                auto it = fingerToKey.find(e.tfinger.fingerId);
                if (it != fingerToKey.end()) {
                    int idx = it->second;
                    if (keys[idx].isInside(mx, my)) {
                        keys[idx].handleEvent(e, winW, winH);
                    } else {
                        bool foundNew = false;
                        for (int i = 0; i < (int)keys.size(); i++) {
                            if (keys[i].isInside(mx, my)) {
                                keys[idx].handleEvent(makeFingerUp(e.tfinger.fingerId), winW, winH);
                                SDL_Event fakeDown = makeFingerDown(e.tfinger);
                                keys[i].handleEvent(fakeDown, winW, winH);
                                fingerToKey[e.tfinger.fingerId] = i;
                                foundNew = true;
                                break;
                            }
                        }
                        if (!foundNew) keys[idx].handleEvent(e, winW, winH);
                    }
                }
            } else if (e.type == SDL_FINGERUP) {
                auto it = fingerToKey.find(e.tfinger.fingerId);
                if (it != fingerToKey.end()) {
                    keys[it->second].handleEvent(e, winW, winH);
                    fingerToKey.erase(it);
                }
            }
        }
        return false;
    }

    std::vector<Key*> getKeyPtrs() {
        std::vector<Key*> ptrs;
        for (auto& k : keys) ptrs.push_back(&k);
        return ptrs;
    }

    void resize(int winW, int winH) {
        float availableWidth = (float)winW;
        float minGap = 2.0f;

        keyWidth = std::max(45.0f, std::min(120.0f, availableWidth / numKeys));

        float totalKeyWidth = keyWidth * numKeys;
        float remaining = availableWidth - totalKeyWidth;
        float gapCandidate = (numKeys > 1) ? remaining / (numKeys - 1) : 0.0f;

        if (gapCandidate < minGap) {
            gap = minGap;
            keyWidth = (availableWidth - (numKeys - 1) * gap) / numKeys;
        } else {
            gap = gapCandidate;
        }

        keyHeight = winH * 0.6f;
        startX    = (winW - (keyWidth * numKeys + (numKeys - 1) * gap)) * 0.5f;
        yPos      = winH * 0.25f;

        buildKeys();
    }

    // --- Waveform selection (band-limited via WaveSchema) ---
    void setWaveform(const WaveformInfo& wf) {
        for (auto& k : keys) {
            if (wf.name == "Sine")   { k.setOscillator(Key::Sine);   continue; }
            if (wf.name == "Square") { k.setOscillator(Key::Square); continue; }
            if (wf.name == "Saw")    { k.setOscillator(Key::Saw);    continue; }

            std::vector<float> table;

            HarmonicSpec spec;
            if (fetchSpecByName(wf.name, spec)) {
                auto hs = toHarmonics(spec);
                // WaveSchema schema(hs, k.frequency, 48000.0, 12000.0, 0.95f);
                
                // WaveSchema schema(hs, k.frequency, 48000.0, 12000.0);
                double cutoff = std::min(6000.0, k.frequency * 20.0);
                WaveSchema schema(hs, k.frequency, 48000.0, cutoff);

                table = schema.buildTable(k.tableSize);
            } else if (!wf.generator().empty()) {
                auto base = wf.generator();
                if (!base.empty()) {
                    float mx = 0.0f; for (float v : base) mx = std::max(mx, std::abs(v));
                    if (mx > 0.0f) {
                        float cap = 0.95f, s = std::min(1.0f/mx, cap/mx);
                        for (auto& v : base) v *= s;
                    }
                    table = std::move(base);
                }
            }

            if (!table.empty()) k.setWavetable(table);
        }
    }

    // --- Public helper for hit-testing ---
    int pickKey(float mx, float my) const {
        for (int i = 0; i < (int)keys.size(); i++) {
            if (keys[i].hit(mx, my)) return i;
        }
        return -1;
    }

private:
    int numKeys;
    float baseFrequency;
    std::vector<double> ratios;
    std::vector<std::string> labels;
    Key::SourceType defaultWaveform;

    float keyWidth, keyHeight, gap, startX, yPos;
    std::vector<Key> keys;
    std::map<SDL_FingerID, int> fingerToKey;

    AudioBus bus {48000.0f};

    void buildKeys() {
        keys.clear();
        int numRatios = (int)ratios.size();
        float xPos = startX;

        for (int i = 0; i < numKeys; i++) {
            int idx = i % numRatios;
            int octave = i / numRatios;
            double factor = std::pow(2.0, octave);

            double freq = baseFrequency * ratios[idx] * factor;
            std::string label = (idx < (int)labels.size()) ? labels[idx] : "";

            Key k(xPos, yPos, keyWidth, keyHeight, i, label);
            k.setFrequency(freq);
            k.setOscillator(defaultWaveform);

            keys.push_back(std::move(k));
            xPos += keyWidth + gap;
        }
    }

    SDL_Event makeFingerUp(SDL_FingerID fid) {
        SDL_Event e{};
        e.type = SDL_FINGERUP;
        e.tfinger.fingerId = fid;
        e.tfinger.touchId = 0;
        e.tfinger.x = 0;
        e.tfinger.y = 0;
        e.tfinger.dx = 0;
        e.tfinger.dy = 0;
        e.tfinger.pressure = 0;
        return e;
    }

    SDL_Event makeFingerDown(const SDL_TouchFingerEvent& src) {
        SDL_Event e{};
        e.type = SDL_FINGERDOWN;
        e.tfinger.touchId = src.touchId;
        e.tfinger.fingerId = src.fingerId;
        e.tfinger.x = src.x;
        e.tfinger.y = src.y;
        e.tfinger.dx = src.dx;
        e.tfinger.dy = src.dy;
        e.tfinger.pressure = src.pressure;
        return e;
    }

    // --- helpers for spec → harmonics mapping ---
    static std::vector<Harmonic> toHarmonics(const HarmonicSpec& spec) {
        const size_t N = (std::min)(spec.amps.size(), spec.phases.size());
        std::vector<Harmonic> hs; hs.reserve(N);
        for (size_t i = 0; i < N; ++i) hs.push_back({spec.amps[i], spec.phases[i]});
        return hs;
    }

    static bool fetchSpecByName(const std::string& name, HarmonicSpec& out) {
        if (name == "BrighterSine") { out = Waveform::BrighterSineSpec(); return true; }
        if (name == "Dod")          { out = Waveform::DodSpec();          return true; }
        return false;
    }
};
