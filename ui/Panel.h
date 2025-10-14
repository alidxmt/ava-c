#pragma once
#include "UI.h"
#include "Waveform.h"
#include <vector>
#include <algorithm>

// -------------------------
// Minimal vertical slider
// -------------------------
class Slider : public Widget {
public:


    float value;       // 0..1
    std::string label; // bottom text
    NVGcolor fillColor;
    NVGcolor bgColor;

    Slider(float x_, float y_, float w_, float h_, const std::string& lbl)
        : Widget(x_, y_, w_, h_), value(0.5f), label(lbl) {
        fillColor = srgbColor(187,125,128);
        bgColor   = srgbColor(24,26,29);
    }

    void draw(NVGcontext* vg) override {
        // background
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, bgColor);
        nvgFill(vg);

        // disabled threshold
        bool disabled = value < 0.25f;
        NVGcolor curFill = disabled ? srgbColor(80,80,80) : fillColor;

        // fill
        float filledH = value * h;
        nvgBeginPath(vg);
        nvgRect(vg, x, y + (h - filledH), w, filledH);
        nvgFillColor(vg, curFill);
        nvgFill(vg);

        // label inside
        nvgFontFace(vg, "ui");
        nvgFontSize(vg, 12.0f);
        nvgFillColor(vg, srgbColor(217,211,215));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgText(vg, x + w * 0.5f, y + 2.0f, label.c_str(), nullptr);
    }


    
    bool handleEvent(const SDL_Event& e, int winW, int winH) {
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            float mx = e.tfinger.x * winW;
            float my = e.tfinger.y * winH;
            if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
                float rel = 1.0f - ((my - y) / h);
                value = std::clamp(rel, 0.0f, 1.0f);
                return true;
            }
        }
        return false;
    }

    bool enabled() const { return value >= 0.25f; }
    float scaledValue() const {
        if (!enabled()) return 0.0f;
        return (value - 0.25f) / 0.75f; // remap 0.25..1 → 0..1
    }
};

// -------------------------
// Panel: top strip with controls
// -------------------------
class Panel {
public:
    Keyboard& keyboard;  // 🔹 store reference

    bool visible;
    float panelHeightFrac;
    std::vector<Widget*> children;
    TouchSketchGenerator& improv;  // 🔹 reference improv generator

    // Controls
    Slider* tremRate;
    Slider* tremDepth;

    Slider* reverbDecay;
    Slider* reverbMix;
    Slider* roomSize;

    WaveformSelector* oscWave;
    InputField* realField;
    InputField* imagField;

    Selector* modeSelector;  //new
    Button* improvToggle;   // new

    bool improvEnabled = false;  // 🔹 track improviser state


Panel(Keyboard& kb, TouchSketchGenerator& imp, float heightFrac = 0.18f)
    : keyboard(kb),
      improv(imp),                 // ✅ initialize reference
      visible(false), panelHeightFrac(heightFrac),
      tremRate(nullptr), tremDepth(nullptr),
      reverbDecay(nullptr), reverbMix(nullptr), roomSize(nullptr),
      oscWave(nullptr), realField(nullptr), imagField(nullptr),
      modeSelector(nullptr), improvToggle(nullptr),
      improvEnabled(false) {}


    void layout(int winW, int winH) {
        for (auto* c : children) delete c;
        children.clear();

        float topOffset = 0.05f * winH;

        // Sizes
        float sliderW   = (2.0f/150.0f) * winW;
        float sliderH   = (10.0f/150.0f) * winW;
        float boxH      = winH * 0.04f;
        float selectorW = winW * 0.12f;
        float inputW    = winW * 0.20f;
        float spacing   = winW * 0.01f;

        float baselineY = topOffset + sliderH + 16.0f;
        float ySlider   = baselineY - sliderH;
        float yBox      = baselineY - boxH;

        float x = spacing;

        // Tremolo (always sine)
        tremRate   = new Slider(x, ySlider, sliderW, sliderH, "Rate");
        children.push_back(tremRate); x += sliderW + spacing;

        tremDepth  = new Slider(x, ySlider, sliderW, sliderH, "Depth");
        children.push_back(tremDepth); x += sliderW + spacing;

        // Reverb
        reverbDecay = new Slider(x, ySlider, sliderW, sliderH, "Decay");
        children.push_back(reverbDecay); x += sliderW + spacing;

        reverbMix   = new Slider(x, ySlider, sliderW, sliderH, "Mix");
        children.push_back(reverbMix); x += sliderW + spacing;

        roomSize    = new Slider(x, ySlider, sliderW, sliderH, "Room");
        children.push_back(roomSize); x += sliderW + spacing;



        realField  = new InputField(x, yBox/2, inputW, boxH);
        // ✅ Start with default harmonics
        realField->text = "1.0 0.5 0.25 0.125";
        children.push_back(realField); 

        imagField  = new InputField(x+inputW + spacing, yBox/2, inputW, boxH);
        // ✅ Start with zeros for imaginary
        imagField->text = "0 0 0 0";
        children.push_back(imagField);
        
        // Oscillator
        oscWave    = new WaveformSelector(x, yBox, selectorW, boxH);
        children.push_back(oscWave); x += selectorW + spacing;


        




        // 🔹 Add Mode selector new 
        modeSelector = new Selector(x, yBox, selectorW * 2.25f, boxH, {});
                children.push_back(modeSelector);
                x += selectorW * 2.25f + spacing;

        x += selectorW * 0.0f;


        // 🔹 Toggle Calligraphy button
        auto* toggleCalligBtn = new Button(x, yBox, 160.0f, boxH,
                                   keyboard.calligraphyEnabled ? "KhtOn"
                                                               : "KhtOff");



        toggleCalligBtn->onClick = [this, toggleCalligBtn]() {
            keyboard.calligraphyEnabled = !keyboard.calligraphyEnabled;
            toggleCalligBtn->text = keyboard.calligraphyEnabled ? "KhtOn"
                                                                : "KhtOff";
        };
        children.push_back(toggleCalligBtn);
        x += 160 + spacing;



       // 🔹 Toggle Improviser button
        improvToggle = new Button(x, yBox, 160.0f, boxH,
                                improvEnabled ? "ImprovOn" : "ImprovOff");

        improvToggle->onClick = [this]() {
            improvEnabled = !improvEnabled;
            improvToggle->text = improvEnabled ? "ImprovOn" : "ImprovOff";
            if (!improvEnabled) {
                improv.releaseAll();   // ✅ stop notes when turned off
            }
        };

        children.push_back(improvToggle);
        x += 160 + spacing;




        // // Real / Imag
        // realField  = new InputField(x, yBox, inputW, boxH);
        // realField->text = "Real";
        // children.push_back(realField); x += inputW + spacing;

        // imagField  = new InputField(x, yBox, inputW, boxH);
        // imagField->text = "Imag";
        // children.push_back(imagField);


        // --- Hook custom waveform generation on blur ---
        auto updateCustomWaveform = [this]() {
            if (oscWave && oscWave->selected().name == "Custom") {
                auto parseList = [](const std::string& txt) {
                    std::vector<float> vals;
                    std::stringstream ss(txt);
                    float v;
                    while (ss >> v) vals.push_back(v);
                    return vals;
                };

                std::vector<float> real = parseList(realField->text);
                std::vector<float> imag = parseList(imagField->text);

                // ✅ Pad shorter list with zeros
                if (real.size() < imag.size()) {
                    real.resize(imag.size(), 0.0f);
                } else if (imag.size() < real.size()) {
                    imag.resize(real.size(), 0.0f);
                }

                auto table = Waveform::buildTable(real, imag, 2048);

                WaveformInfo customWF { "Custom", [table]() { return table; } };

                // ✅ Use the reference we already have
                keyboard.setWaveform(customWF);

            }
        };



        // Hook both fields to regenerate on blur
        realField->onBlur = [updateCustomWaveform](const std::string&) { updateCustomWaveform(); };
        imagField->onBlur = [updateCustomWaveform](const std::string&) { updateCustomWaveform(); };





        
    }

    void toggle() { visible = !visible; }

    void drawFrame(NVGcontext* vg, float x, float y, float w, float h, const char* title) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 6.0f);
        nvgFillColor(vg, srgbColor(24,26,29));
        nvgFill(vg);
        nvgStrokeColor(vg, srgbColor(43,57,86));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);

        nvgFontFace(vg, "ui");
        nvgFontSize(vg, 12.0f);
        nvgFillColor(vg, srgbColor(217,211,215));
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(vg, x + 6.0f, y + 2.0f, title, nullptr);
    }

    void draw(NVGcontext* vg) {
        if (!visible || children.empty()) return;

        // Tremolo frame
        if (tremRate && tremDepth) {
            float x = tremRate->x - 6;
            float w = (tremDepth->x + tremDepth->w + 6) - x;
            float h = std::max(tremRate->h, tremDepth->h) + 20;
            drawFrame(vg, x, tremRate->y - 20, w, h, "Tremolo");
        }

        // Reverb frame
        if (reverbDecay && reverbMix && roomSize) {
            float x = reverbDecay->x - 6;
            float w = (roomSize->x + roomSize->w + 6) - x;
            float h = std::max({ reverbDecay->h, reverbMix->h, roomSize->h }) + 20;
            drawFrame(vg, x, reverbDecay->y - 20, w, h, "Reverb");
        }

        // Oscillator frame
        if (oscWave) {
            float x = oscWave->x - 6;
            float w = oscWave->w + 12;
            if (oscWave->selected().name == "Custom" && realField && imagField) {
                w = (imagField->x + imagField->w + 6) - x;
            }
            float h = oscWave->h + 20;
            drawFrame(vg, x, oscWave->y - 20, w, h, "Oscillator");
        }
        // Mode frame
        if (modeSelector) {
            float x = modeSelector->x - 6;
            float w = modeSelector->w + 12;
            float h = modeSelector->h + 20;
            drawFrame(vg, x, modeSelector->y - 20, w, h, "Mode");
        }

        // Draw children
        for (auto* c : children) {
            if ((c == realField || c == imagField) &&
                oscWave->selected().name != "Custom") {
                continue;
            }
            c->draw(vg);
        }
    }

    bool handleEvent(const SDL_Event& e, int winW, int winH) {
        if (!visible) return false;
        for (auto* c : children) {
            if (c->handleEvent(e)) return true;
            if (auto* s = dynamic_cast<Slider*>(c)) {
                if (s->handleEvent(e, winW, winH)) return true;
            }
            if (auto* w = dynamic_cast<WaveformSelector*>(c)) {
                if (w->handleEvent(e, winW, winH)) return true;
            }
            if (auto* sel = dynamic_cast<Selector*>(c)) {
                if (sel->handleEvent(e)) return true;
            }

        }
        return false;
    }

    ~Panel() {
        for (auto* c : children) delete c;
        children.clear();
    }

        // 🔹 Helpers for main.cpp
        bool tremoloEnabled() const { return tremDepth && tremDepth->enabled(); }
        float tremoloRate()   const { return tremRate ? tremRate->scaledValue() : 0.0f; }
        float tremoloDepth()  const { return tremDepth ? tremDepth->scaledValue() : 0.0f; }

        bool reverbEnabled()       const { return reverbMix && reverbMix->enabled(); }
        float reverbDecayValue()   const { return reverbDecay ? reverbDecay->scaledValue() : 0.0f; }
        float reverbMixValue()     const { return reverbMix ? reverbMix->scaledValue() : 0.0f; }
        float reverbRoomSizeValue()const { return roomSize ? roomSize->scaledValue() : 0.0f; }
        bool improviserEnabled() const {
            return improvEnabled;
        }



};



