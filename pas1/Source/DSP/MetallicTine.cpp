#include "MetallicTine.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {
namespace {
constexpr double twoPi = 6.28318530717958647692;
constexpr std::array<float, 6> tineRatios{1.0f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f};
}

void MetallicTine::trigger(float frequency, float sampleRate, float brightness,
                           float decaySeconds, float resonance, float inharmonicity,
                           float strike) noexcept {
    if (!std::isfinite(frequency) || !std::isfinite(sampleRate) || sampleRate <= 0.0f) {
        reset();
        return;
    }

    const auto safeRate = std::max(sampleRate, 8000.0f);
    const auto safeBrightness = std::clamp(std::isfinite(brightness) ? brightness : 0.55f,
                                           0.0f, 1.0f);
    const auto safeDecay = std::clamp(std::isfinite(decaySeconds) ? decaySeconds : 1.4f,
                                      0.08f, 8.0f);
    const auto safeResonance = std::clamp(std::isfinite(resonance) ? resonance : 0.6f,
                                          0.0f, 1.0f);
    const auto safeInharmonicity = std::clamp(
        std::isfinite(inharmonicity) ? inharmonicity : 0.7f, 0.0f, 1.0f);
    const auto safeStrike = std::clamp(std::isfinite(strike) ? strike : 0.55f, 0.0f, 1.0f);

    float amplitudeSum = 0.0f;
    for (std::size_t index = 0; index < modes_.size(); ++index) {
        auto& mode = modes_[index];
        const auto harmonic = static_cast<float>(index + 1);
        mode.ratio = harmonic + (tineRatios[index] - harmonic) * safeInharmonicity;

        const auto position = static_cast<float>(index)
                             / static_cast<float>(modes_.size() - 1);
        const auto spectralTilt = std::exp(
            -static_cast<float>(index)
                * (1.65f - 1.1f * safeBrightness - 0.35f * safeResonance));
        const auto strikeShape = 0.68f + safeStrike * position * 0.9f;
        mode.amplitude = spectralTilt * strikeShape;
        amplitudeSum += mode.amplitude;

        const auto partialDecay =
            safeDecay / (1.0f + 0.65f * static_cast<float>(index) * (1.0f - safeResonance));
        mode.decayMultiplier = std::exp(-1.0f / (safeRate * partialDecay));
        mode.phaseSine = 0.0;
        mode.phaseCosine = 1.0;
    }

    const auto normalization = amplitudeSum > 0.0f ? 0.88f / amplitudeSum : 0.0f;
    for (auto& mode : modes_)
        mode.amplitude *= normalization;

    updateRotations(frequency, safeRate);
    samplesUntilRetune_ = 16;
    active_ = frequency > 0.0f;
}

void MetallicTine::updateRotations(float frequency, float sampleRate) noexcept {
    const auto safeFrequency = static_cast<double>(frequency);
    const auto safeRate = static_cast<double>(sampleRate);
    for (auto& mode : modes_) {
        const auto partialFrequency = std::min(safeFrequency * mode.ratio, safeRate * 0.45);
        const auto phaseStep = twoPi * partialFrequency / safeRate;
        mode.rotationSine = std::sin(phaseStep);
        mode.rotationCosine = std::cos(phaseStep);
    }
}

float MetallicTine::nextSample(float frequency, float sampleRate) noexcept {
    if (!active_ || !std::isfinite(frequency) || !std::isfinite(sampleRate)
        || frequency <= 0.0f || sampleRate <= 0.0f)
        return 0.0f;

    if (samplesUntilRetune_ <= 0) {
        updateRotations(frequency, sampleRate);
        samplesUntilRetune_ = 16;
    }
    --samplesUntilRetune_;

    float output = 0.0f;
    float remainingEnergy = 0.0f;

    for (auto& mode : modes_) {
        if (mode.amplitude <= 1.0e-7f)
            continue;

        output += static_cast<float>(mode.phaseSine) * mode.amplitude;
        const auto nextSine = mode.phaseSine * mode.rotationCosine
                              + mode.phaseCosine * mode.rotationSine;
        const auto nextCosine = mode.phaseCosine * mode.rotationCosine
                                - mode.phaseSine * mode.rotationSine;
        mode.phaseSine = nextSine;
        mode.phaseCosine = nextCosine;
        mode.amplitude *= mode.decayMultiplier;
        remainingEnergy += mode.amplitude;
    }

    if (remainingEnergy < 1.0e-5f)
        active_ = false;

    return std::isfinite(output) ? output : 0.0f;
}

void MetallicTine::reset() noexcept {
    for (auto& mode : modes_)
        mode.amplitude = 0.0f;
    samplesUntilRetune_ = 0;
    active_ = false;
}

} // namespace pas1::dsp
