#include "Oscillator.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {

void Oscillator::reset(double phase) noexcept {
    phase_ = phase - std::floor(phase);
}

float Oscillator::polyBlep(double phase, double phaseIncrement) noexcept {
    if (phaseIncrement <= 0.0)
        return 0.0f;

    if (phase < phaseIncrement) {
        const auto t = phase / phaseIncrement;
        return static_cast<float>(t + t - t * t - 1.0);
    }

    if (phase > 1.0 - phaseIncrement) {
        const auto t = (phase - 1.0) / phaseIncrement;
        return static_cast<float>(t * t + t + t + 1.0);
    }

    return 0.0f;
}

float Oscillator::nextSample(float frequency, float sampleRate, Waveform waveform,
                             float pulseWidth) noexcept {
    if (!std::isfinite(frequency) || !std::isfinite(sampleRate) || sampleRate <= 0.0f)
        return 0.0f;

    const auto safeFrequency = std::clamp(frequency, 0.0f, sampleRate * 0.45f);
    const auto increment = static_cast<double>(safeFrequency / sampleRate);
    const auto currentPhase = phase_;
    float sample = 0.0f;

    switch (waveform) {
        case Waveform::sine:
            sample = static_cast<float>(std::sin(currentPhase * 6.28318530717958647692));
            break;
        case Waveform::triangle:
            sample = static_cast<float>(1.0 - 4.0 * std::abs(currentPhase - 0.5));
            break;
        case Waveform::saw:
            sample = static_cast<float>(2.0 * currentPhase - 1.0)
                     - polyBlep(currentPhase, increment);
            break;
        case Waveform::square: {
            const auto safePulseWidth = std::clamp(std::isfinite(pulseWidth) ? pulseWidth : 0.5f,
                                                   0.02f, 0.98f);
            auto shiftedPhase = currentPhase - safePulseWidth;
            if (shiftedPhase < 0.0)
                shiftedPhase += 1.0;
            sample = currentPhase < safePulseWidth ? 1.0f : -1.0f;
            sample += polyBlep(currentPhase, increment);
            sample -= polyBlep(shiftedPhase, increment);
            break;
        }
    }

    phase_ += increment;
    if (phase_ >= 1.0)
        phase_ -= 1.0;

    return std::isfinite(sample) ? sample : 0.0f;
}

} // namespace pas1::dsp
