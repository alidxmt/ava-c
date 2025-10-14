#include "AudioEngine.h"
#include <iostream>
#include <algorithm>
#include "../ui/Key.h"

using namespace ava::audio;

AudioEngine::AudioEngine() {
    if (dac.getDeviceCount() < 1) {
        std::cerr << "No audio devices found!\n";
        return;
    }

    // Init oscillator (fallback)
    osc.Init(sampleRate);
    osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    osc.SetFreq(440.0f);
    osc.SetAmp(0.5f);

    // Init tremolo LFO
    tremLFO.Init(sampleRate);
    tremLFO.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    tremLFO.SetFreq(tremRate);
    tremLFO.SetAmp(1.0f);

    // Init reverb
    reverb.Init(sampleRate);
    reverb.SetFeedback(reverbDecay);
    reverb.SetLpFreq(8000.0f);

    RtAudio::StreamParameters oParams;
    oParams.deviceId = dac.getDefaultOutputDevice();
    oParams.nChannels = 2;
    oParams.firstChannel = 0;

    try {
        dac.openStream(&oParams, nullptr, RTAUDIO_FLOAT32,
                       sampleRate, &bufferFrames,
                       &AudioEngine::audioCallback, this);
        dac.startStream();
    }
    catch (const std::exception& e) {
        std::cerr << "RtAudio error: " << e.what() << "\n";
    }
}

AudioEngine::~AudioEngine() {
    stop();
}

void AudioEngine::start() {
    if (!dac.isStreamRunning()) dac.startStream();
}

void AudioEngine::stop() {
    if (dac.isStreamRunning()) dac.stopStream();
    // if (dac.isStreamOpen()) dac.closeStream();
}

// --- Panel parameter setters ---
void AudioEngine::setTremoloRate(float r) {
    // map slider 0..1 → 0.1..10 Hz
    tremRate = 0.1f + r * 9.9f;
    tremLFO.SetFreq(tremRate);
}

void AudioEngine::setTremoloDepth(float d) {
    tremDepth = std::clamp(d, 0.0f, 1.0f);
}

void AudioEngine::setTremoloWaveform(int w) {
    tremWaveform = w;
    switch (w) {
        case 0: tremLFO.SetWaveform(daisysp::Oscillator::WAVE_SIN); break;
        case 1: tremLFO.SetWaveform(daisysp::Oscillator::WAVE_TRI); break;
        case 2: tremLFO.SetWaveform(daisysp::Oscillator::WAVE_SQUARE); break;
        case 3: tremLFO.SetWaveform(daisysp::Oscillator::WAVE_SAW); break;
        default: tremLFO.SetWaveform(daisysp::Oscillator::WAVE_SIN); break;
    }
}

void AudioEngine::setReverbDecay(float d) {
    reverbDecay = std::clamp(d, 0.0f, 0.99f);
}

void AudioEngine::setReverbMix(float m) {
    wetMix = std::clamp(m, 0.0f, 1.0f);
    dryMix = 1.0f - wetMix;
}

void AudioEngine::setReverbRoomSize(float r) {
    roomSize = r;
    // not currently mapped → you can use to scale reverb params if desired
}

void AudioEngine::setCustomHarmonics(const std::vector<float>& real,
                                     const std::vector<float>& imag) {
    harmonicsReal = real;
    harmonicsImag = imag;
    // TODO: pass into your custom oscillator if Key::Custom is active
}

// --- Audio Callback ---
int AudioEngine::audioCallback(void* outputBuffer, void*,
                               unsigned int nFrames, double,
                               RtAudioStreamStatus status, void* userData) {
    auto* engine = static_cast<AudioEngine*>(userData);
    float* out = static_cast<float*>(outputBuffer);

    if (status) std::cerr << "Stream underflow detected!\n";

    for (unsigned int i = 0; i < nFrames; i++) {
        float drySignal = 0.0f;

        // Sum all keys
        for (auto* k : engine->keys) {
            if (k) drySignal += k->process(engine->sampleRate);
        }

        if (engine->keys.empty() && engine->key) {
            drySignal = engine->key->process(engine->sampleRate);
        }

        if (!engine->key && engine->keys.empty()) {
            drySignal = engine->osc.Process();
        }

        // --- Tremolo (amplitude modulation) ---
        float lfo = engine->tremLFO.Process();   // -1..1
        float mod = 0.5f * (lfo + 1.0f);         // → 0..1
        float trem = 1.0f - engine->tremDepth + engine->tremDepth * mod;
        drySignal *= trem;

        // --- Reverb ---
        float wetL = 0.0f, wetR = 0.0f;
        engine->reverb.SetFeedback(engine->reverbDecay);
        engine->reverb.Process(drySignal, drySignal, &wetL, &wetR);

        // --- Mix dry + wet ---
        out[i * 2 + 0] = engine->dryMix * drySignal + engine->wetMix * wetL;
        out[i * 2 + 1] = engine->dryMix * drySignal + engine->wetMix * wetR;
    }
    return 0;
}
