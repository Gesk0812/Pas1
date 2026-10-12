#pragma once

#include <cstdint>

namespace pas1::dsp {

class Portamento {
public:
    void reset(float frequencyHz) noexcept;
    void glideTo(float targetFrequencyHz, std::uint64_t durationSamples) noexcept;
    float nextSample() noexcept;
    float currentFrequency() const noexcept { return currentFrequencyHz_; }
    bool isGliding() const noexcept { return elapsedSamples_ < durationSamples_; }

private:
    static float sanitizeFrequency(float frequencyHz) noexcept;

    float startFrequencyHz_ = 440.0f;
    float targetFrequencyHz_ = 440.0f;
    float currentFrequencyHz_ = 440.0f;
    std::uint64_t elapsedSamples_ = 0;
    std::uint64_t durationSamples_ = 0;
};

} // namespace pas1::dsp
