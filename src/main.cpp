// --- graphics includes (unchanged) ---
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glad/gl.h>
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

// --- audio includes ---
#include <atomic>
#include <cmath>
#include <iostream>
#include <exception>
#include <rtaudio/RtAudio.h> // if this path squiggles, try "RtAudio.h"

// avoid M_PI portability issues
static constexpr double TWO_PI = 6.283185307179586476925286766559;

// shared audio state
struct AudioState {
    std::atomic<double> freq{440.0};
    std::atomic<double> gain{0.2};
    double phase = 0.0;
    double sampleRate = 48000.0;
};

// RtAudio callback (stereo float32)
static int audioCallback(void* outputBuffer, void*, unsigned int nFrames,
                         double /*streamTime*/, RtAudioStreamStatus status, void* userData)
{
    if (status) std::cerr << "RtAudio: stream under/overflow\n";
    auto* state = static_cast<AudioState*>(userData);
    float* out = static_cast<float*>(outputBuffer);

    double phase = state->phase;
    const double sr = state->sampleRate;
    const double f  = state->freq.load(std::memory_order_relaxed);
    const double g  = state->gain.load(std::memory_order_relaxed);
    const double inc = TWO_PI * f / sr;

    for (unsigned int i = 0; i < nFrames; ++i) {
        const float s = static_cast<float>(std::sin(phase) * g);
        out[2*i + 0] = s; // L
        out[2*i + 1] = s; // R
        phase += inc;
        if (phase >= TWO_PI) phase -= TWO_PI;
    }
    state->phase = phase;
    return 0;
}

int main(int, char**) {
    // --- SDL / OpenGL init (unchanged) ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return -1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* win = SDL_CreateWindow("AVA • SDL2 + NanoVG + RtAudio",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!win) return -2;

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) return -3;

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) return -4;

    NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!vg) return -5;

    // --- RtAudio start (new) ---
    RtAudio dac;
    AudioState audio;
    bool audioOk = true;
    try {
        RtAudio::StreamParameters outParams;
        outParams.deviceId = dac.getDefaultOutputDevice();
        outParams.nChannels = 2; // stereo
        unsigned int bufferFrames = 256;
        const unsigned int sr = static_cast<unsigned int>(audio.sampleRate);

        dac.openStream(&outParams, nullptr, RTAUDIO_FLOAT32, sr, &bufferFrames,
                       &audioCallback, &audio);
        dac.startStream();
        std::cout << "Audio running: " << sr << " Hz, buffer " << bufferFrames
                  << " | Keys: Up/Down=Freq  Left/Right=Gain  Esc=Quit\n";
    } catch (const std::exception& e) {
        std::cerr << "Audio disabled: " << e.what() << "\n";
        audioOk = false;
    } catch (...) {
        std::cerr << "Audio disabled: unknown error\n";
        audioOk = false;
    }

    // --- main loop (unchanged + a few keys) ---
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_UP:    audio.freq = audio.freq.load()*1.05946309436; break; // +1 semitone
                    case SDLK_DOWN:  audio.freq = audio.freq.load()/1.05946309436; break; // -1 semitone
                    case SDLK_RIGHT: {
                        double g = audio.gain.load(); g = std::min(1.0, g + 0.02); audio.gain = g; break;
                    }
                    case SDLK_LEFT:  {
                        double g = audio.gain.load(); g = std::max(0.0, g - 0.02); audio.gain = g; break;
                    }
                }
            }
        }

        glViewport(0, 0, 800, 600);
        glClearColor(0.12f, 0.12f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        nvgBeginFrame(vg, 800, 600, 1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, 100, 100, 240, 140);
        nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
        nvgFill(vg);

        // show current freq/gain as a simple bar (optional visual feedback)
        nvgBeginPath(vg);
        nvgRect(vg, 100, 270, (float)(audio.gain.load()*400.0), 12);
        nvgFillColor(vg, nvgRGBA(120, 200, 255, 255));
        nvgFill(vg);
        nvgEndFrame(vg);

        SDL_GL_SwapWindow(win);
    }

    // --- shutdown ---
    if (audioOk) {
        try {
            if (dac.isStreamRunning()) dac.stopStream();
            if (dac.isStreamOpen()) dac.closeStream();
        } catch (...) {}
    }
    nvgDeleteGL3(vg);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
