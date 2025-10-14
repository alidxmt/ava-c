#pragma once
#include <atomic>
#include <algorithm>


namespace ava::dsp {


inline float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }


struct AtomicFloat {
std::atomic<float> v{0.0f};
AtomicFloat() = default;
explicit AtomicFloat(float init) : v(init) {}
float get() const noexcept { return v.load(std::memory_order_relaxed); }
void set(float x) noexcept { v.store(x, std::memory_order_relaxed); }
};


} // namespace ava::dsp