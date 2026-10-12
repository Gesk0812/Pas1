#include "LowFrequencyOscillator.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {
namespace {
constexpr double twoPi = 6.28318530717958647692;
}

void LowFrequencyOscillator::reset(std::uint32_t seed, double phase) noexcept {
    phase_ = std::isfinite(phase) ? phase - std::floor(phase) : 0.0;
    randomState_ = seed == 0 ? 0x9e3779b9u : seed;
    heldValue_ = nextRandomBipolar();
    nextValue_ = nextRandomBipolar();
}

float LowFrequencyOscillator::nextRandomBipolar() noexcept {
    randomState_ ^= randomState_ << 13;
    randomState_ ^= randomState_ >> 17;
    randomState_ ^= randomState_ << 5;
    const auto normalized = static_cast<float>(randomState_ >> 8) / 16777215.0f;
    return normalized * 2.0f - 1.0f;
}

float LowFrequencyOscillator::nextSample(float frequencyHz, float sampleRate,
                                         LfoWaveform waveform) noexcept {
    if (!std::isfinite(frequencyHz) || !std::isfinite(sampleRate) || sampleRate <= 0.0f)
        return 0.0f;

    const auto increment = std::clamp(static_cast<double>(frequencyHz) / sampleRate, 0.0, 0.49);
    const auto currentPhase = phase_;
    float result = 0.0f;

    switch (waveform) {
        case LfoWaveform::sine:
            result = static_cast<float>(std::sin(twoPi * currentPhase));
            break;
        case LfoWaveform::triangle:
            result = static_cast<float>(1.0 - 4.0 * std::abs(currentPhase - 0.5));
            break;
        case LfoWaveform::saw:
            result = static_cast<float>(2.0 * currentPhase - 1.0);
            break;
        case LfoWaveform::square:
            result = currentPhase < 0.5 ? 1.0f : -1.0f;
            break;
        case LfoWaveform::sampleAndHold:
            result = heldValue_;
            break;
        case LfoWaveform::randomSmooth:
            result = heldValue_ + (nextValue_ - heldValue_) * static_cast<float>(currentPhase);
            break;
    }

    phase_ += increment;
    if (phase_ >= 1.0) {
        phase_ -= 1.0;
        heldValue_ = nextValue_;
        nextValue_ = nextRandomBipolar();
    }

    return std::isfinite(result) ? std::clamp(result, -1.0f, 1.0f) : 0.0f;
}

} // namespace pas1::dsp
