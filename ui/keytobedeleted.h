#pragma once
#include "UI.h"
#include "Waveform.h"
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <SDL.h>
#include "daisysp.h"
#include <glad/gl.h>

// -------------------------------------------------------------
// Key  —  GPU-accelerated rendering version
// -------------------------------------------------------------
class Key : public Rect {
public:
    enum SourceType { Sine, Square, Saw, Wavetable };
    void setOscillator(SourceType type) {
        source = type;
        osc = std::make_unique<daisysp::Oscillator>();
        oscDetuned = std::make_unique<daisysp::Oscillator>();
        osc->Init(defaultSampleRate);
        oscDetuned->Init(defaultSampleRate);

        switch (type) {
            case Sine:
                osc->SetWaveform(daisysp::Oscillator::WAVE_SIN);
                oscDetuned->SetWaveform(daisysp::Oscillator::WAVE_SIN);
                break;
            case Square:
                osc->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
                oscDetuned->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
                break;
            case Saw:
                osc->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
                oscDetuned->SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
                break;
            default:
                break;
        }

        osc->SetAmp(0.5f);
        oscDetuned->SetAmp(0.5f);
    }

    void setFrequency(double freq) {
        frequency = freq;
        phaseInc = (frequency / defaultSampleRate) * (double)tableSize;
        if (osc) osc->SetFreq(frequency);
        if (oscDetuned) oscDetuned->SetFreq(frequency);
    }

    SourceType source = Wavetable;
    void setWavetable(const std::vector<float>& table) {
        source = Wavetable;
        osc.reset();
        oscDetuned.reset();
        wavetable = table;
        tableSize = table.size();
        phase = 0.0;
        phaseDetuned = 0.0;
    }

    std::unique_ptr<daisysp::Oscillator> osc;
    std::unique_ptr<daisysp::Oscillator> oscDetuned;
    std::vector<float> wavetable;
    size_t tableSize = 2048;
    double phase = 0.0;
    double phaseInc = 0.0;
    double phaseDetuned = 0.0;

    double frequency = 440.0;
    double defaultSampleRate = 48000.0;
    float gain = 0.0f;
    float lastGain = 0.0f;
    float targetGain = 0.0f;
    float attackTime = 0.22f;
    float releaseTime = 0.35f;
    bool active = false;

    enum EnvState { Idle, Attack, Sustain, Release };
    EnvState envState = Idle;

    // Tremolo
    float tremRateParam  = 0.0f;
    float tremDepthParam = 0.0f;

    // Detune
    float detuneRangeCents = 0.0f;
    float detuneAmount = 0.0f;

    Key(float x, float y, float w, float h,
        int circleNum = 0, const std::string& txt = "")
        : Rect(x, y, w, h, 0.0f, circleNum, txt)
    {
        setOscillator(Sine);
        setFrequency(frequency);
    }

    // -------------------------------------------------------------
    // Fletcher–Munson equal-loudness weighting (ISO 226 approx)
    // -------------------------------------------------------------
    inline float equalLoudnessWeight(float freqHz) const {
        if (freqHz < 20.0f) freqHz = 20.0f;
        if (freqHz > 20000.0f) freqHz = 20000.0f;
        double f2 = freqHz * freqHz;
        double num = pow(12200.0, 2) * f2 * f2;
        double den = (f2 + pow(20.6, 2))
                   * sqrt((f2 + pow(107.7, 2)) * (f2 + pow(737.9, 2)))
                   * (f2 + pow(12200.0, 2));
        double Ra = num / den;
        double AdB = 20.0 * log10(Ra) + 2.0;
        return pow(10.0, -(AdB * 0.3) / 20.0);
    }

    // -------------------------------------------------------------
    // Audio buffer mix
    // -------------------------------------------------------------
    void addToBuffer(float* buffer, int numFrames, double sampleRate = 48000.0) {
        for (int i = 0; i < numFrames; i++)
            buffer[i] += process(sampleRate);
    }
    // -------------------------------------------------------------
    // GPU fields
    // -------------------------------------------------------------
private:
    GLuint vao = 0, vbo = 0, shader = 0;
    GLint uPosLoc = -1, uSizeLoc = -1, uColorLoc = -1;

    void initGPU()
    {
        if (vao) return; // already initialized

        const char* vsSrc = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            uniform vec2 uPos;
            uniform vec2 uSize;
            void main() {
                vec2 pos = uPos + aPos * uSize;
                gl_Position = vec4((pos.x/640.0*2.0-1.0),
                                   -(pos.y/480.0*2.0-1.0),
                                   0.0, 1.0);
            }
        )";

        const char* fsSrc = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main() {
                FragColor = uColor;
            }
        )";

        // Simple quad geometry
        float verts[8] = { 0,0, 1,0, 0,1, 1,1 };

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsSrc, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsSrc, nullptr);
        glCompileShader(fs);

        shader = glCreateProgram();
        glAttachShader(shader, vs);
        glAttachShader(shader, fs);
        glLinkProgram(shader);
        glDeleteShader(vs);
        glDeleteShader(fs);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        uPosLoc   = glGetUniformLocation(shader, "uPos");
        uSizeLoc  = glGetUniformLocation(shader, "uSize");
        uColorLoc = glGetUniformLocation(shader, "uColor");
    }

public:
    // -------------------------------------------------------------
    // GPU-accelerated draw
    // -------------------------------------------------------------
    void draw(NVGcontext* vg) override {
        initGPU();

        float maxIntensity = 0.0f;
        for (auto &kv : activeTouches)
            maxIntensity = std::max(maxIntensity, kv.second);
        NVGcolor c = hoverColor;
        c.a = maxIntensity;

        // GPU rect fill
        glUseProgram(shader);
        glBindVertexArray(vao);
        glUniform2f(uPosLoc, x, y);
        glUniform2f(uSizeLoc, w, h);
        glUniform4f(uColorLoc, c.r, c.g, c.b, c.a);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(0);

        // Keep NanoVG text / circle drawing
        Rect::draw(vg);
    }

    ~Key() {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (shader) glDeleteProgram(shader);
    }

        // --- Allow move, disallow copy ---
    Key(const Key&) = delete;
    Key& operator=(const Key&) = delete;
    Key(Key&&) noexcept = default;
    Key& operator=(Key&&) noexcept = default;

    // -------------------------------------------------------------
    // Original audio + event system
    // -------------------------------------------------------------
    bool handleEvent(const SDL_Event& e, int winW, int winH) override {
        float mx = 0, my = 0;
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            mx = e.tfinger.x * winW;
            my = e.tfinger.y * winH;
        }

        if (e.type == SDL_FINGERDOWN) {
            if (isInside(mx, my)) {
                float intensity = computeIntensity(my);
                detuneAmount = computeDetune(mx);
                activeTouches[e.tfinger.fingerId] = intensity;
                noteOn(intensity);
                return true;
            }
        }
        if (e.type == SDL_FINGERMOTION) {
            if (activeTouches.find(e.tfinger.fingerId) != activeTouches.end()) {
                if (isInside(mx, my)) {
                    float intensity = computeIntensity(my);
                    detuneAmount = computeDetune(mx);
                    activeTouches[e.tfinger.fingerId] = intensity;
                    noteMove(intensity);
                    return true;
                } else {
                    activeTouches.erase(e.tfinger.fingerId);
                    if (activeTouches.empty()) noteOff();
                }
            }
        }
        if (e.type == SDL_FINGERUP) {
            activeTouches.erase(e.tfinger.fingerId);
            if (activeTouches.empty()) noteOff();
            return true;
        }
        return false;
    }

    float process(double sampleRate = 48000.0) {
        lastGain = gain;
        if (!active) return 0.0f;

        float stepAttack  = 1.0f / (attackTime * sampleRate);
        switch (envState) {
            case Attack:
                gain += stepAttack;
                if (gain >= targetGain) {
                    gain = targetGain;
                    envState = Sustain;
                }
                break;
            case Sustain:
                gain += (targetGain - gain) * 0.002f;
                break;
            case Release:
                gain *= 0.9995f;
                if (gain <= 0.0001f) {
                    gain = 0;
                    envState = Idle;
                    active = false;
                }
                break;
            case Idle:
            default:
                return 0.0f;
        }

        float sample = 0.0f;
        float ratio  = powf(2.0f, detuneAmount / 1200.0f);
        if (osc && oscDetuned) {
            osc->SetFreq(frequency);
            oscDetuned->SetFreq(frequency * ratio);
            sample = 0.5f * (osc->Process() + oscDetuned->Process());
        } else if (!wavetable.empty()) {
            size_t idx1 = (size_t)phase % tableSize;
            float s1 = wavetable[idx1];
            phaseDetuned += (frequency * ratio / sampleRate) * tableSize;
            if (phaseDetuned >= (double)tableSize)
                phaseDetuned -= (double)tableSize;
            size_t idx2 = (size_t)phaseDetuned % tableSize;
            float s2 = wavetable[idx2];
            sample = 0.5f * (s1 + s2);
            phase += phaseInc;
            if (phase >= (double)tableSize)
                phase -= (double)tableSize;
        }

        lastRawSample = sample;
        return sample * gain * equalLoudnessWeight((float)frequency);
    }

    bool isInside(float mx, float my) const {
        return (mx >= x && mx <= x + w && my >= y && my <= y + h);
    }

    bool hit(float mx, float my) const {
        return (mx >= x && mx <= x + w && my >= y && my <= y + h);
    }

    bool isActive() const { return active; }
    float getGain() const { return gain; }
    double getFrequency() const { return frequency; }
    float getLastSample() const { return lastRawSample; }

private:
    std::map<SDL_FingerID, float> activeTouches;
    float lastRawSample = 0.0f;

    float computeIntensity(float my) {
        float relY = (my - y) / h;
        float d = std::abs(relY - 0.5f) * 2.0f;
        return (expf(-4.0f * d) - expf(-4.0f)) / (1.0f - expf(-4.0f));
    }

    float computeDetune(float mx) {
        float relX = (mx - x) / w;
        float centered = (relX - 0.5f) * 2;
        return centered * detuneRangeCents;
    }

    void noteOn(float relGain) { targetGain = relGain; envState = Attack; active = true; }
    void noteMove(float relGain) { targetGain = relGain; }
    void noteOff() { envState = Release; }
};
