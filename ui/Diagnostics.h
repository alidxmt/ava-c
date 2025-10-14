#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include "Key.h"

class Diagnostics {
public:
    static void start(const std::vector<Key*>& keys,
                      int   periodMs       = 200,
                      float clipHeadroom   = 0.9f,
                      int   voicesWarn     = 8,
                      float gainJumpWarn   = 0.20f,
                      float sampleJumpWarn = 0.70f,   // abs(sample diff) per tick
                      int   jankWarnMs     = 30)      // timer drift warning
    {
        stop();

        // Make a **copy** of the pointers so we don't reference a dead vector.
        auto tracked = keys;

        stopFlag = false;
        diagThread = std::thread([=]() mutable {
            using clock = std::chrono::steady_clock;
            auto expected = clock::now() + std::chrono::milliseconds(periodMs);

            // per-key previous values for deltas
            std::unordered_map<const Key*, float> prevGain;
            std::unordered_map<const Key*, float> prevSample;

            while (!stopFlag) {
                // timer & jank
                std::this_thread::sleep_until(expected);
                auto now = clock::now();
                auto driftMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - expected).count();
                expected += std::chrono::milliseconds(periodMs);
                if (driftMs > jankWarnMs) {
                    std::cout << "[CPU_JANK] driftMs=" << driftMs << std::endl;
                }

                // count active + sum gain
                int active = 0;
                float sumGain = 0.0f;
                for (auto* k : tracked) {
                    if (!k) continue;
                    if (k->isActive()) { active++; sumGain += k->getGain(); }
                }

                if (active >= voicesWarn) {
                    std::cout << "[CLIP_RISK/OVERLAP] active=" << active
                              << " sumGain=" << sumGain << std::endl;
                }
                if (sumGain > clipHeadroom) {
                    std::cout << "[CLIP_RISK] sumGain=" << sumGain << " > " << clipHeadroom << std::endl;
                }

                // per-key deltas
                for (auto* k : tracked) {
                    if (!k) continue;

                    // gain jump
                    float g  = k->getGain();
                    float pg = prevGain[k];
                    if (std::fabs(g - pg) > gainJumpWarn) {
                        std::cout << "[GAIN_JUMP] f=" << k->getFrequency()
                                  << " gain=" << g
                                  << " d=" << (g - pg) << std::endl;
                    }
                    prevGain[k] = g;

                    // waveform discontinuity (approx, tick-to-tick)
                    float s  = k->getLastSample();
                    float ps = prevSample[k];
                    if (prevSample.find(k) != prevSample.end()) {
                        float ds = std::fabs(s - ps);
                        if (ds > sampleJumpWarn) {
                            std::cout << "[DISCONTINUITY] f=" << k->getFrequency()
                                      << " |Δsample|=" << ds << std::endl;
                        }
                    }
                    prevSample[k] = s;
                }
            }
        });
    }

    static void stop() {
        stopFlag = true;
        if (diagThread.joinable()) diagThread.join();
    }

private:
    static inline std::thread diagThread;
    static inline std::atomic<bool> stopFlag{false};
};
