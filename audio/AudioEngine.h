#pragma once
#include <vector>
#include <memory>
#include <rtaudio/RtAudio.h>
#include "../ui/Key.h"
#include "Effects/reverbsc.h"

// DaisySP includes
#include "daisysp.h"

namespace ava {
namespace audio {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    void start();
    void stop();

    void setKey(Key* k) { key = k; }
    void setKeys(const std::vector<Key*>& ks) { keys = ks; }

    // --- Panel setters ---
    void setTremoloRate(float r);
    void setTremoloDepth(float d);
    void setTremoloWaveform(int w);

    void setReverbDecay(float d);
    void setReverbMix(float m);
    void setReverbRoomSize(float r);

    void setCustomHarmonics(const std::vector<float>& real,
                            const std::vector<float>& imag);

    void clearKeys() {
        keys.clear();
        key = nullptr;
    }

private:
    RtAudio dac;
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 256;

    Key* key = nullptr;
    std::vector<Key*> keys;

    // Core DSP
    daisysp::Oscillator osc;
    daisysp::ReverbSc   reverb;
    daisysp::Oscillator tremLFO;   // tremolo LFO

    // Wet/dry mix
    float dryMix = 0.75f;
    float wetMix = 0.25f;

    // Panel parameters
    float tremRate   = 5.0f;   // Hz
    float tremDepth  = 0.5f;   // 0..1
    int   tremWaveform = 0;    // 0 = sine

    float reverbDecay = 0.85f;
    float roomSize    = 0.5f;

    // Custom harmonics
    std::vector<float> harmonicsReal;
    std::vector<float> harmonicsImag;

    static int audioCallback(void* outputBuffer, void* inputBuffer,
                             unsigned int nFrames, double streamTime,
                             RtAudioStreamStatus status, void* userData);
};

} // namespace audio
} // namespace ava
