#pragma once
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <random>
#include <vector>
#include <memory>
#include <algorithm>
#include <SDL.h>

// ------------------------------------------
// One synthetic "finger stroke"
// ------------------------------------------
class TouchSketch {
public:
    enum class CurveType { Linear, Quadratic, Exponential, Random };

    TouchSketch(float x0, float y0,
                float x1, float y1,
                int durationMs,
                CurveType type = CurveType::Random,
                SDL_FingerID fingerId = (900 + (rand() % 100)))
        : x0(x0), y0(y0), x1(x1), y1(y1),
          duration(durationMs),
          fingerId(fingerId),
          started(false), sentUp(false),
          start(std::chrono::steady_clock::now())
    {
        if (type == CurveType::Random)
            curve = static_cast<CurveType>(rand() % 3);
        else
            curve = type;

        setupCoefficients();
    }

    SDL_FingerID getFingerId() const { return fingerId; }
    std::pair<float,float> getLastPos() const { return position(1.0f); }

    // Call each frame from main loop
    void updateAndSendEvents(int winW, int winH) {
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float,std::milli>(now - start).count();
        float t = elapsed / duration;

        const int SUBSTEPS = 6;  
        for (int i = 0; i < SUBSTEPS; i++) {
            float tt = t - (SUBSTEPS - 1 - i) * (1.0f / (duration * 60.0f));
            if (tt < 0) continue;
            if (tt > 1.0f) tt = 1.0f;

            auto [x,y] = position(tt);

            SDL_Event ev{};
            ev.tfinger.touchId = 1;
            ev.tfinger.fingerId = fingerId;   // ✅ use our own member
            ev.tfinger.timestamp = SDL_GetTicks();
            ev.tfinger.x = x / winW;
            ev.tfinger.y = y / winH;
            ev.tfinger.dx = 0;
            ev.tfinger.dy = 0;
            ev.tfinger.pressure = 1.0f;

            if (!started) {
                ev.type = SDL_FINGERDOWN;
                SDL_PushEvent(&ev);
                started = true;
            } else if (tt < 1.0f) {
                ev.type = SDL_FINGERMOTION;
                SDL_PushEvent(&ev);
            } else if (!sentUp) {
                ev.type = SDL_FINGERUP;
                SDL_PushEvent(&ev);
                sentUp = true;
            }
        }
    }

    bool finished() const { return sentUp; }

private:
    float x0,y0,x1,y1;
    int duration;
    CurveType curve;
    SDL_FingerID fingerId;
    bool started;
    bool sentUp;
    std::chrono::steady_clock::time_point start;

    float a=0,b=0,c=0;
    float k=2.0f;

    std::pair<float,float> position(float t) const {
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        float x = x0 + (x1 - x0) * t;
        float y;
        switch(curve) {
            case CurveType::Linear:      y = y0 + (y1 - y0) * t; break;
            case CurveType::Quadratic:   y = a*t*t + b*t + c; break;
            case CurveType::Exponential: y = y0 + (y1 - y0) * (std::exp(k*t)-1.0f)/(std::exp(k)-1.0f); break;
            default: y = y0 + (y1 - y0) * t;
        }
        return {x,y};
    }

    void setupCoefficients() {
        if (curve == CurveType::Quadratic) {
            c = y0;
            b = 0; 
            a = (y1 - y0);
        }
    }
};

// ------------------------------------------
// Generates a stream of synthetic strokes
// ------------------------------------------
class TouchSketchGenerator {
public:
    TouchSketchGenerator(float winW, float winH)
        : winW(winW), winH(winH),
          rng(std::random_device{}()),
          strokeCount(0),
          pauseUntil(std::chrono::steady_clock::now())
    {
        minX = 0;
        maxX = winW;
        minY = 0;
        maxY = winH;
        resetPhrase();
    }

    void setBand(float topFrac, float bottomFrac) {
        minX = 0;
        maxX = winW;
        minY = winH * topFrac;
        maxY = winH * bottomFrac;
    }

    void setBounds(float x0, float y0, float x1, float y1) {
        minX = x0;
        minY = y0;
        maxX = x1;
        maxY = y1;
    }

    // Release all fake fingers immediately
    void releaseAll() {
        for (auto& s : active) {
            SDL_Event ev{};
            ev.type = SDL_FINGERUP;
            ev.tfinger.fingerId = s->getFingerId();   // ✅ use getter
            auto [xx, yy] = s->getLastPos();
            ev.tfinger.x = xx / winW;
            ev.tfinger.y = yy / winH;
            SDL_PushEvent(&ev);
        }
        active.clear();
    }

    void update(int winW, int winH) {
        this->winW = winW;
        this->winH = winH;

        // 1. Remove finished strokes
        active.erase(
            std::remove_if(active.begin(), active.end(),
                           [](const std::unique_ptr<TouchSketch>& s){ return s->finished(); }),
            active.end()
        );

        auto now = std::chrono::steady_clock::now();
        if (now < pauseUntil) return;

        // 2. Spawn new strokes if fewer than 2 active
        while (active.size() < 2) {
            active.push_back(std::make_unique<TouchSketch>(randomSketch()));
            strokeCount++;

            if (strokeCount >= phraseLength) {
                std::uniform_int_distribution<int> pauseDist(0,1);
                int pauseMs = (pauseDist(rng) == 0 ? 500 : 1000);
                pauseUntil = now + std::chrono::milliseconds(pauseMs);
                resetPhrase();
                break;
            }
        }

        // 3. Update active strokes
        for (auto& s : active) {
            s->updateAndSendEvents(winW, winH);
        }
    }

private:
    float winW, winH;
    float minX, minY, maxX, maxY;
    std::mt19937 rng;

    std::vector<std::unique_ptr<TouchSketch>> active;
    int strokeCount;
    int phraseLength;
    std::chrono::steady_clock::time_point pauseUntil;

    void resetPhrase() {
        std::uniform_int_distribution<int> phraseDist(3, 8);
        phraseLength = phraseDist(rng);
        strokeCount = 0;
    }

    TouchSketch randomSketch() {
        std::vector<int> durations = {2000,2000,2000,2000,2000,2000};
        std::discrete_distribution<int> durDist({2,2,2,1,1,1});
        int dur = durations[durDist(rng)];

        std::uniform_real_distribution<float> xDist(minX, maxX);
        std::uniform_real_distribution<float> yDist(minY, maxY);
        float x0 = xDist(rng);
        float y0 = yDist(rng);

        std::uniform_real_distribution<float> dxDist(-0.15f*winW, 0.15f*winW);
        std::uniform_real_distribution<float> dyDist(-0.2f*winH, 0.2f*winH);
        float x1 = std::clamp(x0 + dxDist(rng), minX, maxX);
        float y1 = std::clamp(y0 + dyDist(rng), minY, maxY);

        std::uniform_int_distribution<int> curveDist(0,2);
        auto type = static_cast<TouchSketch::CurveType>(curveDist(rng));

        return TouchSketch(x0, y0, x1, y1, dur, type);
    }
};
