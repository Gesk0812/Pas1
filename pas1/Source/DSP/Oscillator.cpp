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
        case Waveform::square:
        case Waveform::pulse: {
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
        case Waveform::reverseSaw:
            sample = -static_cast<float>(2.0 * currentPhase - 1.0)
                     + polyBlep(currentPhase, increment);
            break;
        case Waveform::organ:
        case Waveform::hollow:
        case Waveform::formant:
        case Waveform::bell:
        case Waveform::metallic:
        case Waveform::clipped: {
            const auto phaseAngle = currentPhase * 6.28318530717958647692;
            const auto maxRatio = std::min(24.0, 0.45 / std::max(increment, 1.0e-12));
            double sum = 0.0;
            double weightSum = 0.0;
            const auto addPartial = [&](double ratio, double weight) {
                if (ratio <= maxRatio) {
                    sum += std::sin(phaseAngle * ratio) * weight;
                    weightSum += std::abs(weight);
                }
            };
            switch (waveform) {
                case Waveform::organ:
                    for (int harmonic = 1; harmonic <= 12; ++harmonic)
                        addPartial(harmonic, 1.0 / std::pow(static_cast<double>(harmonic), 0.72));
                    break;
                case Waveform::hollow:
                    addPartial(1.0, 0.34);
                    addPartial(3.0, 1.0);
                    addPartial(5.0, 0.52);
                    addPartial(7.0, 0.28);
                    addPartial(9.0, 0.16);
                    break;
                case Waveform::formant:
                    addPartial(1.0, 0.38);
                    addPartial(2.0, 0.5);
                    addPartial(3.0, 0.9);
                    addPartial(4.0, 1.0);
                    addPartial(5.0, 0.7);
                    addPartial(6.0, 0.28);
                    addPartial(8.0, 0.16);
                    break;
                case Waveform::bell:
                    addPartial(1.0, 1.0);
                    addPartial(2.0, 0.54);
                    addPartial(3.01, 0.31);
                    addPartial(4.18, 0.2);
                    addPartial(5.43, 0.12);
                    break;
                case Waveform::metallic:
                    addPartial(1.0, 0.8);
                    addPartial(1.41, 1.0);
                    addPartial(2.76, 0.62);
                    addPartial(4.93, 0.43);
                    addPartial(6.6, 0.26);
                    addPartial(8.21, 0.16);
                    break;
                case Waveform::clipped:
                    addPartial(1.0, 1.0);
                    addPartial(3.0, 0.34);
                    addPartial(5.0, 0.16);
                    addPartial(7.0, 0.08);
                    break;
                default:
                    break;
            }
            sample = weightSum > 0.0
                         ? static_cast<float>(sum / weightSum * 1.65)
                         : 0.0f;
            break;
        }
        case Waveform::count:
            sample = 0.0f;
            break;
    }

    phase_ += increment;
    if (phase_ >= 1.0)
        phase_ -= 1.0;

    return std::isfinite(sample) ? std::clamp(sample, -1.0f, 1.0f) : 0.0f;
}

} // namespace pas1::dsp
