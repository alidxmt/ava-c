#pragma once
#include <cmath>
#include <cstdint>


namespace ava::dsp {


class Oscillator {
public:
void setSampleRate(double sr) { sampleRate_ = (sr > 0.0 ? sr : 48000.0); }
void setFrequency(double hz) { freq_ = (hz > 1e-6 ? hz : 1e-6); updateInc(); }


// Returns next sample in [-1, 1]
float process() {
// naive sine — bandlimited oscillators come later
const float y = std::sin(phase_);
phase_ += inc_;
if (phase_ > twoPi) phase_ -= twoPi;
return y;
}


private:
void updateInc() { inc_ = static_cast<float>(twoPi * freq_ / sampleRate_); }


static constexpr float twoPi = 6.28318530717958647692f;
double sampleRate_ = 48000.0;
double freq_ = 440.0;
float phase_ = 0.0f;
float inc_ = static_cast<float>(twoPi * 440.0 / 48000.0);
};


} // namespace ava::dsp