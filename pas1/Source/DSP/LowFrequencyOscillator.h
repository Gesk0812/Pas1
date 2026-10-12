#pragma once

#include <cstdint>

namespace pas1::dsp {

enum class LfoWaveform {
    sine = 0,
    triangle,
    saw,
    square,
    sampleAndHold,
    randomSmooth
};

class LowFrequencyOscillator {
public:
    void reset(std::uint32_t seed = 0x9e3779b9u, double phase = 0.0) noexcept;
    float nextSample(float frequencyHz, float sampleRate, LfoWaveform waveform) noexcept;

private:
    float nextRandomBipolar() noexcept;

    double phase_ = 0.0;
    float heldValue_ = 0.0f;
    float nextValue_ = 0.0f;
    std::uint32_t randomState_ = 0x9e3779b9u;
};

} // namespace pas1::dsp
