#pragma once
#include <nanovg.h>
#include <SDL.h>
#include <string>
#include <functional>
#include <cmath>
#include <map>
#include <algorithm>
#include <array>
#include <cstdio>   // for snprintf


// -------------------------
// sRGB Helpers (for consistent colors)
// -------------------------
inline float srgbToLinear(float c) {
    c /= 255.0f;
    return (c <= 0.04045f) ? (c / 12.92f)
                           : powf((c + 0.055f) / 1.055f, 2.4f);
}

inline NVGcolor srgbColor(int r, int g, int b, int a = 255) {
    return nvgRGBAf(
        srgbToLinear((float)r),
        srgbToLinear((float)g),
        srgbToLinear((float)b),
        a / 255.0f
    );
}

inline NVGcolor withAlpha(NVGcolor c, float a) { c.a = a; return c; }



inline void drawGrid(NVGcontext* vg, int winW, int winH, int stepPx = 10) {
    nvgBeginPath(vg);

    // ultra-light gray, alpha ~15 (≈6% opacity)
    nvgStrokeColor(vg, nvgRGBA(200, 200, 200, 1));
    nvgStrokeWidth(vg, 1.0f);

    // vertical lines
    for (int x = 0; x <= winW; x += stepPx) {
        nvgMoveTo(vg, (float)x, 0.0f);
        nvgLineTo(vg, (float)x, (float)winH);
    }

    // horizontal lines
    for (int y = 0; y <= winH; y += stepPx) {
        nvgMoveTo(vg, 0.0f, (float)y);
        nvgLineTo(vg, (float)winW, (float)y);
    }

    nvgStroke(vg);
}




// -------------------------
// Default Palette
// -------------------------
struct Palette {
    NVGcolor bgWindow   = srgbColor(24,26,29);        
    NVGcolor bgObjects  = srgbColor(255, 17, 29);
    NVGcolor border     = srgbColor(12,12,16);

    NVGcolor textMain   = srgbColor(217, 211, 215);
    NVGcolor textSubtle = srgbColor(156, 110, 113);

    NVGcolor keyBg      = srgbColor(255,2,3);
    NVGcolor keyBorder  = srgbColor(15,17,29);
    NVGcolor keyHover   = srgbColor(255,0,0);      // Pure red hover

    NVGcolor glowInner  = srgbColor(40, 11, 15);

    int outerAlpha      = 80;
    int innerAlpha      = 100;
};

// -------------------------
// Base widget
// -------------------------
class Widget {
public:
    float x, y, w, h;
    Widget(float x_, float y_, float w_, float h_)
        : x(x_), y(y_), w(w_), h(h_) {}
    virtual void draw(NVGcontext* vg) = 0;
    virtual bool handleEvent(const SDL_Event& e) { return false; }
    virtual ~Widget() = default;
};

// -------------------------
// Label
// -------------------------
class Label : public Widget {
public:
    std::string text;
    NVGcolor textColor, bgColor, borderColor;
    float rotationDeg; // rotation in degrees (default 0)

    Label(float x_, float y_, float w_, float h_, 
          const std::string& t, float rotDeg = 0.0f)
        : Widget(x_, y_, w_, h_), 
          text(t), 
          rotationDeg(rotDeg) 
    {
        textColor   = srgbColor(187,125,128);
        bgColor     = srgbColor(24,26,29);
        borderColor = withAlpha(srgbColor(43,57,86), 0.0f);
    }

    void setText(const std::string& t) { text = t; }
    void setRotation(float deg) { rotationDeg = deg; }

    void draw(NVGcontext* vg) override {
        nvgFontFace(vg, "ui");
        float fs = std::min(h * 0.58f, 22.0f);
        // nvgFontSize(vg, fs);
        nvgFontSize(vg, 24.0f); 

        nvgSave(vg);

        // Translate to center of label
        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;
        nvgTranslate(vg, cx, cy);

        // Rotate if needed
        if (rotationDeg != 0.0f) {
            float angle = rotationDeg * (NVG_PI / 180.0f);
            nvgRotate(vg, angle);
        }

        // Draw text centered
        nvgTextAlign(vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);

        // Shadow
        nvgFillColor(vg, withAlpha(srgbColor(0,0,0), 0.35f));
        nvgText(vg, 1.0f, 1.0f, text.c_str(), nullptr);

        // Text
        nvgFillColor(vg, textColor);
        nvgText(vg, 0, 0, text.c_str(), nullptr);

        nvgRestore(vg);
    }
};


// -------------------------
// Button
// -------------------------
class Button : public Widget {
public:
    std::string text;
    NVGcolor textColor, bgColor, borderColor;
    std::function<void()> onClick;
    bool pressed = false;

    Button(float x_, float y_, float w_, float h_, const std::string& t)
        : Widget(x_, y_, w_, h_), text(t) {
        textColor   = srgbColor(187,125,128);
        bgColor     = srgbColor(24,26,29);
        borderColor = withAlpha(srgbColor(187,125,128), 1.0f);
    }

    void draw(NVGcontext* vg) override {
        float r = 8.0f;

        // Background
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillColor(vg, bgColor);
        nvgFill(vg);

        // Border
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgStrokeColor(vg, borderColor);
        nvgStrokeWidth(vg, .2f);
        nvgStroke(vg);

        // Text
        nvgFontFace(vg, "ui");
        float fs = std::min(h * 0.52f, 20.0f);
        // nvgFontSize(vg, fs);
        nvgFontSize(vg, 14.0f); 
        nvgTextAlign(vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);

        nvgFillColor(vg, textColor);
        nvgText(vg, x + w*0.5f, y + h*0.5f, text.c_str(), nullptr);
    }

    bool handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            float mx = (float)e.button.x, my = (float)e.button.y;
            if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
                pressed = true; return true;
            }
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            float mx = (float)e.button.x, my = (float)e.button.y;
            if (pressed && mx >= x && mx <= x + w && my >= y && my <= y + h) {
                pressed = false;
                if (onClick) onClick();
                return true;
            }
            pressed = false;
        }
        return false;
    }
};

// -------------------------
// InputField
// -------------------------
class InputField : public Widget {
public:
    std::string text;
    bool focused;
    NVGcolor textColor, bgColor, borderColor;

    // 🔹 Callbacks
    std::function<void(const std::string&)> onChange; 
    std::function<void(const std::string&)> onBlur;

    InputField(float x_, float y_, float w_, float h_)
        : Widget(x_, y_, w_, h_), focused(false) {
        textColor   = srgbColor(187,125,128);
        bgColor     = srgbColor(24,26,29);
        borderColor = withAlpha(srgbColor(187,125,128), 1.f);
    }

    void draw(NVGcontext* vg) override {
        float r = 8.0f;

        // Background
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillColor(vg, bgColor);
        nvgFill(vg);

        // Border
        NVGcolor ring = focused ? srgbColor(210,135,138) : borderColor;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgStrokeColor(vg, ring);
        nvgStrokeWidth(vg, 0.2f);
        nvgStroke(vg);

        // Text
        nvgFontFace(vg, "ui");
        nvgFontSize(vg, 24.0f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        const char* content = text.empty() ? "Type…" : text.c_str();
        NVGcolor tc = text.empty() ? withAlpha(textColor, 0.45f) : textColor;

        nvgFillColor(vg, tc);
        nvgText(vg, x + 10.0f, y + h * 0.5f, content, nullptr);
    }

    bool handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            float mx = (float)e.button.x, my = (float)e.button.y;
            bool wasFocused = focused;
            focused = (mx >= x && mx <= x + w && my >= y && my <= y + h);

            // 🔹 If we *lose* focus → fire onBlur
            if (wasFocused && !focused && onBlur) {
                onBlur(text);
            }
            return focused;
        }

        if (!focused) return false;

        if (e.type == SDL_TEXTINPUT) {
            text += e.text.text;
            if (onChange) onChange(text);
            return true;
        }

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_BACKSPACE && !text.empty()) {
                text.pop_back();
                if (onChange) onChange(text);
                return true;
            }
            // Optional: press Enter to also trigger onBlur
            if (e.key.keysym.sym == SDLK_RETURN && onBlur) {
                onBlur(text);
                focused = false;
                return true;
            }
        }
        return false;
    }
};


// -------------------------
// Horizontal Line
// -------------------------
struct HLine : public Widget {
    float thickness;
    NVGcolor color;

    HLine(float x_, float y_, float w_, float thickness_, NVGcolor c)
        : Widget(x_, y_, w_, thickness_), thickness(thickness_), color(c) {}

    void draw(NVGcontext* vg) override {
        nvgBeginPath(vg);
        nvgMoveTo(vg, x, y);
        nvgLineTo(vg, x + w, y);
        nvgStrokeColor(vg, color);
        nvgStrokeWidth(vg, thickness);
        nvgStroke(vg);
    }
};


struct Circles : public Widget {
    int value;                // number to display in binary
    float spacing;            // space between circles
    float radius;             // circle radius
    NVGcolor onColor;         // active color
    NVGcolor offColor;        // inactive color

    Circles(float x_, float y_, float w_, float h_,
            int val = 0, float r = 8.0f, float sp = 4.0f)
        : Widget(x_, y_, w_, h_),
          value(val), spacing(sp), radius(r),
          onColor(srgbColor(213, 173, 163)),  // light color
          offColor(srgbColor(97, 55, 56)) {}  // dark color

    void setValue(int v) { value = v; }

    void draw(NVGcontext* vg) override {
        // 2 columns × 3 rows
        int bit = 1;
        for (int col = 0; col < 2; col++) {
            for (int row = 0; row < 3; row++) {
                float cx = x + col * (2 * radius + spacing) + radius;
                float cy = y + row * (2 * radius + spacing) + radius;

                nvgBeginPath(vg);
                nvgCircle(vg, cx, cy, radius);

                // Check if this bit is ON in value
                if (value & bit) {
                    nvgFillColor(vg, onColor);
                } else {
                    nvgFillColor(vg, offColor);
                }
                nvgFill(vg);

                bit <<= 1; // next binary position
            }
        }
    }
};




// -------------------------
// Rectangle Key
// -------------------------
struct Rect : public Widget {
    NVGcolor bgColor;
    NVGcolor borderColor;
    NVGcolor hoverColor;
    float borderWidth;
    float cornerRadius;
    std::map<SDL_FingerID, float> activeTouches; // intensity per finger
    int circleBinaryNumber; // number displayed as Braille dots
    std::string labelText;  // new: vertical text label

    Rect(float x_, float y_, float w_, float h_, 
         float borderW = 0.0f, int circleNum = 5, 
         const std::string& txt = "")
        : Widget(x_, y_, w_, h_),
          bgColor(srgbColor(5,5,7)),
          borderColor(srgbColor(5,5,7)),
          hoverColor(srgbColor(80, 23, 29)), 
          borderWidth(borderW),
          cornerRadius(10.0f),
          circleBinaryNumber(circleNum),
          labelText(txt) {}

    virtual bool handleEvent(const SDL_Event& e, int winW, int winH) {

        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            float mx = e.tfinger.x * winW;
            float my = e.tfinger.y * winH;

            if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
                float relY = (my - y) / h;
                float d = std::abs(relY - 0.5f) * 2.0f;  
                float intensity = 1.0f - d;
                if (intensity < 0.0f) intensity = 0.0f;

                activeTouches[e.tfinger.fingerId] = intensity;
                return true;
            } else {
                activeTouches.erase(e.tfinger.fingerId);
            }
        }
        if (e.type == SDL_FINGERUP) {
            activeTouches.erase(e.tfinger.fingerId);
        }
        return false;
    }

    void draw(NVGcontext* vg) override {
        float r = cornerRadius;

        // Base background
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillColor(vg, bgColor);
        nvgFill(vg);

        
    // Hover overlay (confined to rect, does not dim circles/text)
    float maxIntensity = 0.0f;
    for (auto &kv : activeTouches) {
        maxIntensity = std::max(maxIntensity, kv.second);
    }
    if (maxIntensity > 0.0f) {
        nvgSave(vg);
        nvgScissor(vg, x, y, w, h); // limit drawing region to the rect only

        NVGcolor c = hoverColor;
        c.a = maxIntensity;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, r);
        nvgFillColor(vg, c);
        nvgFill(vg);

        nvgResetScissor(vg);
        nvgRestore(vg);
    }




        // Border
        if (borderWidth > 0.0f) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, r);
            nvgStrokeColor(vg, borderColor);
            nvgStrokeWidth(vg, borderWidth);
            nvgStroke(vg);
        }


        float radius  = w / 20.0f;

        // --- Draw Braille-like circles ---

        float padding = 4*radius;
        
        float spacing = radius * 1.5f;

        float firstCx = x + padding + 2*radius; // x of first column
        float lastCy  = y + padding + (2 * radius + spacing) * 2 + 12 * radius + 4.0f * radius; // y after last circle

        if (circleBinaryNumber >= 0) {
            int bit = 1;
            for (int col = 0; col < 2; col++) {
                for (int row = 0; row < 3; row++) {
                    float cx = x + padding + col * (2 * radius + spacing) + radius;
                    float cy = y + padding + row * (2 * radius + spacing) + radius + radius * 2;

                    nvgBeginPath(vg);
                    nvgCircle(vg, cx, cy, radius);

                    if (circleBinaryNumber & bit) {
                        nvgFillColor(vg, srgbColor(213, 173, 163)); // ON color
                    } else {
                        nvgFillColor(vg, srgbColor(97, 55, 56));   // OFF color
                    }
                    nvgFill(vg);

                    bit <<= 1;
                }
            }
        }

        // --- Draw vertical text ---
        if (!labelText.empty()) {
            nvgSave(vg);
            // nvgFontFace(vg, "ui");
            nvgFontFace(vg, "ui");

            // nvgFontSize(vg, radius * 4.0f); // text size relative to circles
            nvgFontSize(vg, 12.0f); 
            nvgFillColor(vg, srgbColor(213, 173, 163));
            nvgTextAlign(vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);

            // Position: same x as first circle, just below last circle
            nvgTranslate(vg, firstCx, lastCy + radius * 2.0f + radius * 2.0f);
            nvgRotate(vg, 3*-NVG_PI / 2.0f); // rotate 90° counter-clockwise
            
            labelText.resize(16, ' ');

            nvgText(vg, 20, 0, labelText.c_str(), nullptr);

            
            nvgRestore(vg);
        }
    }
};



// -------------------------
// Simple Selector (cycle through options)
// -------------------------
class Selector : public Widget {
public:
    std::vector<std::string> options;
    int selectedIndex;
    std::function<void(int, const std::string&)> onSelect;

    Selector(float x_, float y_, float w_, float h_,
             const std::vector<std::string>& opts)
        : Widget(x_, y_, w_, h_), options(opts), selectedIndex(0) {}

    void setOptions(const std::vector<std::string>& opts) {
        options = opts;
        if (selectedIndex >= (int)options.size()) selectedIndex = 0;
    }

    std::string selected() const {
        if (options.empty()) return "";
        return options[selectedIndex];
    }

    void draw(NVGcontext* vg) override {
        // Box
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 6.0f);
        nvgFillColor(vg, srgbColor(24,26,29));
        nvgFill(vg);
        nvgStrokeColor(vg, srgbColor(187,125,128));
        nvgStrokeWidth(vg, 0.5f);
        nvgStroke(vg);

        // Current option text
        if (!options.empty()) {
            nvgFontFace(vg, "ui");
            nvgFontSize(vg, 16.0f);
            nvgFillColor(vg, srgbColor(217,211,215));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(vg, x + w * 0.5f, y + h * 0.5f,
                    options[selectedIndex].c_str(), nullptr);
        }
    }

    bool handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            float mx = (float)e.button.x, my = (float)e.button.y;
            if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
                if (!options.empty()) {
                    selectedIndex = (selectedIndex + 1) % options.size();
                    if (onSelect) onSelect(selectedIndex, options[selectedIndex]);
                }
                return true;
            }
        }
        return false;
    }
};


// -------------------------
// FingerStatusBar: show 10 fingers freq,gain
// -------------------------
class FingerStatusBar {
public:
    static const int MaxFingers = 10;

    struct Slot {
        double freq = 0.0;
        float gain = 0.0f;
        bool active = false;
    };

    std::array<Slot, MaxFingers> slots;

    void clear() {
        for (auto& s : slots) s = Slot{};
    }

    // called from Keyboard
    void setFinger(int idx, double freq, float gain) {
        if (idx >= 0 && idx < MaxFingers) {
            slots[idx].freq = freq;
            slots[idx].gain = gain;
            slots[idx].active = true;
        }
    }

    void releaseFinger(int idx) {
        if (idx >= 0 && idx < MaxFingers) {
            slots[idx].active = false;
            slots[idx].freq = 0.0;
            slots[idx].gain = 0.0f;
        }
    }

        void draw(NVGcontext* vg, float winW, float winH) {
        float barH = 28.0f;
        float slotW = winW / MaxFingers;
        float y = winH - barH;

        // background
        nvgBeginPath(vg);
        nvgRect(vg, 0, y, winW, barH);
        nvgFillColor(vg, srgbColor(24,26,29));
        nvgFill(vg);

        // divider lines
        for (int i = 1; i < MaxFingers; i++) {
            float x = i * slotW;
            nvgBeginPath(vg);
            nvgMoveTo(vg, x, y);
            nvgLineTo(vg, x, y + barH);
            nvgStrokeColor(vg, srgbColor(43,57,86));
            nvgStrokeWidth(vg, 1.0f);
            nvgStroke(vg);
        }

        nvgFontFace(vg, "ui");
        nvgFontSize(vg, 12.0f);
        nvgFillColor(vg, srgbColor(217,211,215));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        // collect active slots
        std::vector<Slot> active;
        for (int i = 0; i < MaxFingers; i++) {
            if (slots[i].active) active.push_back(slots[i]);
        }

        // draw 10 visual slots left→right
        for (int i = 0; i < MaxFingers; i++) {
            float cx = i * slotW + slotW * 0.5f;
            float cy = y + barH * 0.5f;

            if (i < (int)active.size()) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.0f,%.2f",
                         active[i].freq, active[i].gain);
                nvgText(vg, cx, cy, buf, nullptr);
            } else {
                nvgText(vg, cx, cy, "| , |", nullptr);
            }
        }
    }

};
