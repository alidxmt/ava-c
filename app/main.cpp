#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include "Calligraphy.h" 
#include "WaveDiagram.h"
#include "Keyboard.h"
#include "Mode.h"
#include "improviser.h"
#include "Diagnostics.h"
#include "TouchCrosshair.h"
#include "Key.h"
#include <glad/gl.h>
#include "json.hpp"
#include "ModeLoader.h"
#include <nanovg.h>
#include <nanovg_gl.h>
#include "core/EventRouter.h"
#include "audio/AudioEngine.h"
#include "UI.h"
#include "Panel.h"
#include <iostream>
#include <string>
#include <curl/curl.h>


#define NVG_GL3 1

////windows touch default cancelling
#ifdef _WIN32
#include <windows.h>
#include <winuser.h>

// Some SDKs don't define this symbol, so define it if missing
#ifndef SPI_SETPENPRESSANDHOLD
#define SPI_SETPENPRESSANDHOLD 0x00000000  // dummy fallback (won’t be called)
#endif

// Completely disable Windows touch feedback (light rectangles, ripples, etc.)
static void DisableWindowsTouchFeedback()
{
    BOOL disable = TRUE;

    // Disable system-level touch and gesture visualization (available on Win8+)
    SystemParametersInfo(SPI_SETCONTACTVISUALIZATION, 0, (PVOID)(ULONG_PTR)disable, 0);
    SystemParametersInfo(SPI_SETGESTUREVISUALIZATION, 0, (PVOID)(ULONG_PTR)disable, 0);

    // Try disabling pen press-and-hold gesture (if supported)
#ifdef SPI_SETPENPRESSANDHOLD
    SystemParametersInfo(SPI_SETPENPRESSANDHOLD, 0, (PVOID)(ULONG_PTR)disable, 0);
#endif

    // Disable per-window feedback (Win8+)
    HWND hwnd = GetActiveWindow();
    if (hwnd)
    {
        SetWindowFeedbackSetting(hwnd, FEEDBACK_TOUCH_CONTACTVISUALIZATION, 0, sizeof(disable), &disable);
        SetWindowFeedbackSetting(hwnd, FEEDBACK_TOUCH_TAP, 0, sizeof(disable), &disable);
        SetWindowFeedbackSetting(hwnd, FEEDBACK_TOUCH_DOUBLETAP, 0, sizeof(disable), &disable);
        SetWindowFeedbackSetting(hwnd, FEEDBACK_TOUCH_PRESSANDHOLD, 0, sizeof(disable), &disable);
        SetWindowFeedbackSetting(hwnd, FEEDBACK_PEN_BARRELVISUALIZATION, 0, sizeof(disable), &disable);
    }
}
#endif

////end windows


using json = nlohmann::json;

TouchCrosshair crosshair;


extern "C" {
    NVGcontext* nvgCreateGL3(int flags);
    void        nvgDeleteGL3(NVGcontext* ctx);
}

// Core + Audio
using ava::audio::AudioEngine;




// -------------------------
// Units helper
// -------------------------
struct Units {
    float winW, winH;
    float dpi;
    Units(float w, float h, float d) : winW(w), winH(h), dpi(d) {}
    float percentW(float p) const { return p * winW; }
    float percentH(float p) const { return p * winH; }
    float dp(float d) const { return d * (dpi / 96.0f); }
    float cm(float c) const { return (c / 2.54f) * dpi; }
};



// -------------------------
// Init SDL + GL
// -------------------------
static bool initGL(SDL_Window*& window, SDL_GLContext& glctx,
                   int& winW, int& winH,
                   int& fbW, int& fbH,
                   float& pixelRatio) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

    window = SDL_CreateWindow(
        "AVA-C", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        0, 0,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!window) return false;

    glctx = SDL_GL_CreateContext(window);
    if (!glctx) return false;
    SDL_GL_MakeCurrent(window, glctx);
    SDL_GL_SetSwapInterval(1);

    if (gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0) return false;

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_FRAMEBUFFER_SRGB);

    SDL_GetWindowSize(window, &winW, &winH);
    SDL_GL_GetDrawableSize(window, &fbW, &fbH);
    pixelRatio = (float)fbW / (float)winW;

    return true;
}



// --- Keyboard Layout Helper ---
    void layoutKeyboard(Keyboard& kb, int winW, int winH, const Mode& mode, int numKeys = 30) {
    float gap = 0.0f; // min 2px gap between keys

    // compute key width so that left/right padding == key width
    float keyWidth = (winW - (numKeys - 1) * gap) / (numKeys + 2);
    float keyHeight = winH * 0.66f;   // 75% height
    float startX = keyWidth;          // padding = key width
    float yPos = winH * 0.2f;

    kb = Keyboard(
        numKeys,
        55.0,
        mode.ratios,
        mode.labels,
        Key::Sine,
        keyWidth,
        keyHeight,
        gap,       // ✅ proper gap
        startX,
        yPos
    );
}


// helper json
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Callback to capture curl response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch JSON from API
json fetch_json(const std::string& name, const std::string& pin, const std::string& file) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");

    std::string response;
    std::string postData = R"({"name":")" + name + R"(","pin":")" + pin + R"(","file":")" + file + R"("})";

    curl_easy_setopt(curl, CURLOPT_URL, "https://ava-api.doxameter.com/api/get_json");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) 
        throw std::runtime_error(curl_easy_strerror(res));

    return json::parse(response);
}


//end json helper


// -------------------------
// Main
// -------------------------
int main(int argc, char* argv[]) {
    

   // Declare JSON variables
    json dastanj;

    try {
        // Fetch dastan
        dastanj = fetch_json("ali", "1234", "dastan");
        if (dastanj.is_array() && !dastanj.empty()) {
            std::cout << "First element of dastan:\n";
            std::cout << dastanj[0].dump(4) << "\n";
        } else {
            std::cout << "No elements found in dastan.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error fetching JSON: " << e.what() << "\n";
    }



    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    #ifdef _WIN32
        // 🚫 Disable Windows Ink / Touch visualization feedback
        DisableWindowsTouchFeedback();
    #endif

    SDL_Window* window = nullptr;
    SDL_GLContext glctx = nullptr;
    int winW = 0, winH = 0;
    int fbW = 0, fbH = 0;
    float pxRatio = 1.0f;

    if (!initGL(window, glctx, winW, winH, fbW, fbH, pxRatio))
        return EXIT_FAILURE;

    float ddpi=96, hdpi=96, vdpi=96;
    SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);

    NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!vg) return EXIT_FAILURE;

    int fontUi = nvgCreateFont(vg, "ui", "assets/Inconsolata-Light.ttf");
    // int fontUi = nvgCreateFont(vg, "ui", "assets/FreeFarsi-Mono.ttf");

    if (fontUi == -1) {
        std::fprintf(stderr, "Failed to load Inconsolata-Light.ttf\n");
        return EXIT_FAILURE;
    }

    

    Calligraphy calligraphy(winW, winH);   // ✅ New Calligraphy
    bool calligraphyEnabled = true;

    bool running = true;
    SDL_Event e;
    EventRouter router(&running);
    SDL_StartTextInput();

    float avgDpi = (hdpi + vdpi) * 0.5f;
    Units u(winW, winH, avgDpi);

    Palette p;
    HLine headerDivider(0, u.percentH(0.15f), winW, 2.0f, p.border);

   



    // --- Keyboard Mode from Babel ---
// Mode mode = Mode::Babel()[0];   // get the first mode in Babel

    // --- Keyboard Mode: 12 Equal Temperament ---
    // Mode mode = Mode::equalTemperament(12, "12-TET");


std::vector<Mode> modes;
try {
    // modes = ModeLoader::fromJSON("assets/modes_eng.json");
    // modes = ModeLoader::fromJSON("assets/dastandataeng.json");
    modes = ModeLoader::fromJSON(dastanj);

    std::cout << "Loaded " << modes.size() << " modes\n";
} catch (const std::exception& ex) {
    std::cerr << "JSON load failed: " << ex.what() << std::endl;
    return EXIT_FAILURE;
}

    // // load JSON modes
    // auto modes = ModeLoader::fromJSON("assets/dastandata.json");
    // // auto modes = ModeLoader::fromJSON("assets/modes_eng.json");

    // std::cout << "Loaded " << modes.size() << " modes\n";

    // pick the one you want
    // Mode mode("empty", {}, {}); // placeholder

    // for (const auto& m : modes) {
    //     if (m.name == "|2|24|چهارگاه..مخالف") {
    //         mode = m;
    //         mode.debugPrint("Selected", 30, 110.0);

    //         break;
    //     }
    // }

    // Mode mode("empty", {}, {});

    // // start with the first mode in the JSON (if available)
    // if (!modes.empty()) {
    //     mode = modes[0];
    //     mode.debugPrint("Initial", 30, 110.0);
    // }
    Mode mode = Mode::equalTemperament(12, "|ET|12-TET");

    // now build keyboard from that mode
    Keyboard keyboard(
        30,                // numKeys
        55.0,             // base frequency
        mode.ratios,       // ✅ from JSON
        mode.labels,       // ✅ from JSON
        Key::Sine,
        0, 0, 0,
        0, 0
    );

    layoutKeyboard(keyboard, winW, winH, mode, 30);

    AudioEngine audio;
        // Force tremolo waveform to Sine
    audio.setKeys(keyboard.getKeyPtrs());

    audio.setTremoloWaveform(0);

    // --- Global AudioBus tuning ---
    keyboard.setMasterGain(0.8f);
    keyboard.setLimiterThreshold(0.98f);
    keyboard.setLowpassHz(6000.0f);       // safe cutoff
    keyboard.setLowpassQ(0.707f);         // smooth slope
    keyboard.setLowpassEnabled(true);     // enable LPF

    // --- Keyboard Mode from JSON ---
    // Mode mode("empty", {}, {}); // placeholder

    // for (const auto& m : modes) {
    //     if (m.name == "|گویا|چهارگاه..درآمد اول") {
    //         mode = m;
    //         break;
    //     }
    // }


    // Keyboard keyboard(
    //     30,                // numKeys
    //     110.0,             // base frequency
    //     mode.ratios,       // ✅ from Babel
    //     mode.labels,       // ✅ from Babel
    //     Key::Sine,         // default waveform
    //     0, 0, 0,           // key width, height, gap
    //     0, 0               // startX, yPos
    // );

    // layoutKeyboard(keyboard, winW, winH, mode, 30);

    // ✅ Start diagnostics on all keys
    // Diagnostics::start(keyboard.getKeyPtrs(), 200, 0.9f, 8, 0.2f);

    keyboard.resize(winW, winH);

   

    TouchSketchGenerator improvLeft(winW, winH);
    // TouchSketchGenerator improvRight(winW, winH);

    improvLeft.setBounds(.03125*winW, winH * 0.25f, winW * 0.5f, winH * 0.40f);
    // improvRight.setBounds(winW * 0.5f, winH * 0.25f, winW, winH * 0.85f);

    


    


    // --- Panel ---
// Panel panel;
// panel.layout(winW, winH);

// // 🔹 Connect callbacks first
// auto connectPanel = [&]() {
//     // panel.oscWave->onSelect = [&](int idx) {
//     //     auto wf = panel.oscWave->selected();
//     //     keyboard.setWaveform(wf);
//     // };
//     panel.oscWave->onSelect = [&](int idx) {
//         auto wf = panel.oscWave->selected();

//         if (wf.name == "Custom") {
//             // Default real/imag
//             std::vector<float> real = {1.0f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.01f, 0.001f};
//             std::vector<float> imag = {0,0,0,0,0,0,0,0};

//             // Build wavetable
//             auto table = Waveform::buildTable(real, imag, 2048);

//             // Wrap into WaveformInfo
//             WaveformInfo customWF {"Custom", [table]() { return table; }};

//             // Assign to keyboard
//             keyboard.setWaveform(customWF);
//         } else {
//             // Normal case
//             keyboard.setWaveform(wf);
//         }
//     };

// };
// connectPanel();


// --- Panel ---
Panel panel(keyboard, improvLeft);
panel.layout(winW, winH);


FingerStatusBar fingerBar;   // ✅ new

    // --- 🔹 Define helper here ---
    auto populateModeSelector = [&](Panel& panel, const std::vector<Mode>& allModes) {
        if (!panel.modeSelector) return;

        std::vector<std::string> names;
        for (auto& m : allModes) {
            names.push_back(m.name);
        }

        panel.modeSelector->setOptions(names);

        panel.modeSelector->onSelect = [&](int idx, const std::string&) {
            if (idx < 0 || idx >= static_cast<int>(allModes.size())) return;
                
            // ✅ Stop using old keys before building new ones
            audio.clearKeys();
            audio.stop();

            mode = allModes[idx];

            // build keyboard with enum waveform type
            keyboard = Keyboard(
                30, 110.0,
                mode.ratios,
                mode.labels,
                Key::Sine,   // just a placeholder
                0, 0, 0,
                0, 0
            );
            layoutKeyboard(keyboard, winW, winH, mode, 30);
            audio.setKeys(keyboard.getKeyPtrs());
            // ✅ Restart audio safely with new keys
            audio.start();

            // then sync with panel waveform
            auto wf = panel.oscWave ? panel.oscWave->selected()
                                    : Waveform::available()[0];
            keyboard.setWaveform(wf);

            // mode.debugPrint("Selected", 30, 110.0);
        };

    };


    // --- Initial call ---
    populateModeSelector(panel, modes);
    



// 🔹 Connect callbacks
panel.oscWave->onSelect = [&](int idx) {
    auto wf = panel.oscWave->selected();
    if (wf.name != "Custom") {
        keyboard.setWaveform(wf);
    } else {
        // ✅ Ensure defaults appear in the input fields
        if (panel.realField->text == "Real")
            panel.realField->text = "1.0 0.5 0.25 0.125";
        if (panel.imagField->text == "Imag")
            panel.imagField->text = "0 0 0 0";

        auto parseList = [](const std::string& txt) {
            std::vector<float> vals;
            std::stringstream ss(txt);
            float v;
            while (ss >> v) vals.push_back(v);
            return vals;
        };

        auto table = Waveform::buildTable(
            parseList(panel.realField->text),
            parseList(panel.imagField->text),
            2048
        );
        WaveformInfo customWF {"Custom", [table]() { return table; }};
        keyboard.setWaveform(customWF);
    }
};

// connectPanel();




panel.oscWave->currentIndex = 1;
if (panel.oscWave->onSelect)
    panel.oscWave->onSelect(1);
    std::map<SDL_FingerID, int> fingerToKey;
    // --- Loop ---
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            router.processEvent(e);
            crosshair.handleEvent(e, winW, winH);
            // ✅ Calligraphy events
            if (keyboard.calligraphyEnabled) {
                if (e.type == SDL_FINGERDOWN) calligraphy.startStroke(e.tfinger);
                else if (e.type == SDL_FINGERMOTION) calligraphy.moveStroke(e.tfinger);
                else if (e.type == SDL_FINGERUP) calligraphy.endStroke(e.tfinger);
            }
            else {
                calligraphy.clear();
            }
            // ✅ Panel toggle when tapping top 15%
            if (e.type == SDL_FINGERDOWN) {
                float mx = e.tfinger.x * winW;
                float my = e.tfinger.y * winH;
                if (my < 0.05f * winH) {   // top 15% of screen
                    panel.toggle();
                }
            }
            keyboard.handleEvent(e, winW, winH);
            panel.handleEvent(e, winW, winH);
            // ✅ update finger status bar
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
                int idx = keyboard.pickKey(e.tfinger.x * winW, e.tfinger.y * winH);
                if (idx >= 0) {
                    auto keys = keyboard.getKeyPtrs();
                    fingerBar.setFinger(e.tfinger.fingerId % FingerStatusBar::MaxFingers,
                                        keys[idx]->getFrequency(),
                                        keys[idx]->getGain());
                }
            }
            if (e.type == SDL_FINGERUP) {
                fingerBar.releaseFinger(e.tfinger.fingerId % FingerStatusBar::MaxFingers);
            }
            // Resize
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                SDL_GetWindowSize(window, &winW, &winH);
                SDL_GL_GetDrawableSize(window, &fbW, &fbH);
                pxRatio = (float)fbW / (float)winW;
                u.winW = winW; 
                u.winH = winH;

                calligraphy.resize(winW, winH);
                calligraphy.clear();
                headerDivider = HLine(0, u.percentH(0.15f), winW, 2.0f, p.border);
                layoutKeyboard(keyboard, winW, winH, mode, 30);
                // 🔹 update audio with new key pointers
                audio.setKeys(keyboard.getKeyPtrs());
                // Diagnostics::start(keyboard.getKeyPtrs(), 200, 0.9f, 8, 0.2f);
                fingerBar.clear();   // ✅ clear slots on resize
                panel.layout(winW, winH);
                populateModeSelector(panel, modes);
                // re-assign callback after relayout
                panel.oscWave->onSelect = [&](int idx) {
                    auto wf = panel.oscWave->selected();
                    if (wf.name != "Custom") {
                        keyboard.setWaveform(wf);
                    } else {
                        auto parseList = [](const std::string& txt) {
                            std::vector<float> vals;
                            std::stringstream ss(txt);
                            float v;
                            while (ss >> v) vals.push_back(v);
                            return vals;
                        };
                        auto table = Waveform::buildTable(
                            parseList(panel.realField->text),
                            parseList(panel.imagField->text),
                            2048
                        );
                        WaveformInfo customWF {"Custom", [table]() { return table; }};
                        keyboard.setWaveform(customWF);
                    }
                };
                // 🔹 Re-apply BrighterSine after resize
                panel.oscWave->currentIndex = 1;
                if (panel.oscWave->onSelect)
                    panel.oscWave->onSelect(1);
            }
        }
        
        // --- Audio updates with enable/disable logic
        if (panel.tremoloEnabled()) {
            audio.setTremoloRate(panel.tremoloRate());
            audio.setTremoloDepth(panel.tremoloDepth());
        } else {
            audio.setTremoloDepth(0.0f); // disabled
        }
        if (panel.reverbEnabled()) {
            audio.setReverbDecay(panel.reverbDecayValue());
            audio.setReverbMix(panel.reverbMixValue());
            audio.setReverbRoomSize(panel.reverbRoomSizeValue());
        } else {
            audio.setReverbMix(0.0f);
        }
        if (panel.improviserEnabled()) {
            improvLeft.update(winW, winH);
        } else {
            improvLeft.releaseAll();
        }
        // improvRight.update(winW, winH);

        // --- Draw ---
        glViewport(0, 0, fbW, fbH);
        glClearColor(p.bgWindow.r, p.bgWindow.g, p.bgWindow.b, p.bgWindow.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgBeginFrame(vg, winW, winH, pxRatio);
        headerDivider.draw(vg);
        keyboard.draw(vg);
        panel.draw(vg);
        if (keyboard.calligraphyEnabled) {
            calligraphy.draw(vg);
        }
        fingerBar.draw(vg, winW, winH);
        crosshair.draw(vg, winW, winH);
        drawGrid(vg, winW, winH, 10.0);
        nvgEndFrame(vg);
        SDL_GL_SwapWindow(window);
    }

    nvgDeleteGL3(vg);
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    // ✅ Stop diagnostics thread
    // Diagnostics::stop();

    return EXIT_SUCCESS;
}
