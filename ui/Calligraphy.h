#pragma once

#include <SDL.h>
#include <nanovg.h>
#include <vector>
#include <unordered_map>
#include <cmath>

// Stroke point
struct StrokePoint {
    float x, y, w;
    NVGcolor color;
    Uint64 t;
    float leftX, leftY;
    float rightX, rightY;
};

// Stroke record
struct StrokeRecord {
    std::vector<StrokePoint> pts;
    bool fading = false;
    Uint64 endTime = 0;
};

class Calligraphy {
public:
    Calligraphy(int w, int h) : winW(w), winH(h) {}

    void resize(int w, int h) { winW = w; winH = h; }

    inline bool insideArea(float px, float py) const {
        return (px >= 0.05f * winW && px <= 0.95f * winW &&
                py >= 0.25f * winH && py <= 0.85f * winH);
    }

    inline void startStroke(SDL_TouchFingerEvent& e) {
        float px = e.x * winW;
        float py = e.y * winH;
        if (!insideArea(px, py)) return;

        StrokeRecord rec;
        rec.fading = false;
        rec.endTime = 0;
        activeStrokes[e.fingerId] = std::move(rec);
    }

    inline void moveStroke(SDL_TouchFingerEvent& e) {
        float px = e.x * winW;
        float py = e.y * winH;
        if (!insideArea(px, py)) return;

        auto it = activeStrokes.find(e.fingerId);
        if (it == activeStrokes.end()) return;

        addPoint(it->second, e.x, e.y, SDL_GetTicks64());
    }

    inline void endStroke(SDL_TouchFingerEvent& e) {
        auto it = activeStrokes.find(e.fingerId);
        if (it == activeStrokes.end()) return;

        it->second.fading = true;
        it->second.endTime = SDL_GetTicks64() + fadeDurationMs;
        finishedStrokes.push_back(std::move(it->second));
        activeStrokes.erase(it);
    }

    inline void clear() {
        activeStrokes.clear();
        finishedStrokes.clear();
    }

    inline void draw(NVGcontext* vg) {
        Uint64 now = SDL_GetTicks64();

        for (auto& [fid, rec] : activeStrokes)
            drawStroke(vg, rec, 1.0f);

        for (auto it = finishedStrokes.begin(); it != finishedStrokes.end();) {
            if (it->fading && now >= it->endTime) {
                it = finishedStrokes.erase(it);
            } else {
                float alpha = 1.0f;
                if (it->fading) {
                    Uint64 remain = it->endTime - now;
                    alpha = remain / (float)fadeDurationMs;
                    if (alpha < 0) alpha = 0;
                }
                drawStroke(vg, *it, alpha);
                ++it;
            }
        }
    }

private:
    int winW, winH;
    Uint64 fadeDurationMs = 4000; // 20 seconds
    std::unordered_map<SDL_FingerID, StrokeRecord> activeStrokes;
    std::vector<StrokeRecord> finishedStrokes;

    inline void addPoint(StrokeRecord& rec, float xNorm, float yNorm, Uint64 now) {
        float px = xNorm * winW;
        float py = yNorm * winH;

        if (!rec.pts.empty()) {
            auto& last = rec.pts.back();
            float dx = px - last.x;
            float dy = py - last.y;
            if ((dx*dx + dy*dy) < 1.5f*1.5f) return;
        }

        float w = 6.0f;
        float cosA = 0.5f, sinA = 0.866f;
        float leftX  = px + sinA * w;
        float leftY  = py - cosA * w;
        float rightX = px - sinA * w;
        float rightY = py + cosA * w;

        StrokePoint pt{px, py, w, nvgRGBA(230, 100, 20, 255), now,
                       leftX, leftY, rightX, rightY};
        rec.pts.push_back(pt);
    }

    inline void drawStroke(NVGcontext* vg, const StrokeRecord& rec, float alpha) {
        if (rec.pts.size() < 2) return;

        NVGcolor c = rec.pts[0].color;
        c.a *= alpha;
        nvgFillColor(vg, c);

        nvgBeginPath(vg);
        for (size_t i = 1; i < rec.pts.size(); ++i) {
            auto& A = rec.pts[i - 1];
            auto& B = rec.pts[i];
            nvgMoveTo(vg, A.leftX, A.leftY);
            nvgLineTo(vg, B.leftX, B.leftY);
            nvgLineTo(vg, B.rightX, B.rightY);
            nvgLineTo(vg, A.rightX, A.rightY);
            nvgClosePath(vg);
        }
        nvgFill(vg);
    }
};
