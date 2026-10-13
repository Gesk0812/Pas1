#include "AncientInstrument.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {
namespace {
constexpr double twoPi = 6.28318530717958647692;

constexpr std::array<float, 10> aulosWeights{
    1.0f, 0.48f, 0.36f, 0.28f, 0.21f, 0.16f, 0.52f, 0.28f, 0.19f, 0.13f
};
constexpr std::array<float, 10> lyreRatios{
    1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f
};
constexpr std::array<float, 10> lyreWeights{
    1.0f, 0.58f, 0.34f, 0.23f, 0.16f, 0.12f, 0.09f, 0.07f, 0.055f, 0.04f
};
constexpr std::array<float, 10> gongRatios{
    1.0f, 1.47f, 1.92f, 2.31f, 2.78f, 3.42f, 4.15f, 5.06f, 6.21f, 7.83f
};
constexpr std::array<float, 10> gongWeights{
    1.0f, 0.74f, 0.56f, 0.42f, 0.32f, 0.24f, 0.18f, 0.14f, 0.11f, 0.08f
};
constexpr std::array<float, 10> foundMetalRatios{
    1.0f, 1.39f, 1.76f, 2.48f, 3.17f, 3.93f, 4.76f, 5.82f, 7.11f, 8.63f
};
constexpr std::array<float, 10> foundMetalWeights{
    1.0f, 0.78f, 0.61f, 0.46f, 0.34f, 0.26f, 0.2f, 0.15f, 0.11f, 0.08f
};

float bounded(float value, float fallback, float minimum, float maximum) noexcept {
    return std::clamp(std::isfinite(value) ? value : fallback, minimum, maximum);
}
} // namespace

void AncientInstrumentVoice::trigger(AncientInstrument instrument, float frequency,
                                     float sampleRate, float brightness,
                                     float decaySeconds, float air, float pipeDetune,
                                     std::uint32_t noiseSeed) noexcept {
    const auto mode = static_cast<int>(instrument);
    if (mode <= static_cast<int>(AncientInstrument::off)
        || mode > static_cast<int>(AncientInstrument::foundMetal)
        || !std::isfinite(frequency) || frequency <= 0.0f
        || !std::isfinite(sampleRate) || sampleRate <= 0.0f) {
        reset();
        return;
    }

    instrument_ = instrument;
    const auto safeRate = std::max(sampleRate, 8000.0f);
    const auto safeBrightness = bounded(brightness, 0.62f, 0.0f, 1.0f);
    const auto safeDecay = bounded(decaySeconds, 2.8f, 0.08f, 20.0f);
    air_ = bounded(air, 0.32f, 0.0f, 1.0f);
    const auto detuneRatio = std::pow(
        2.0f, bounded(pipeDetune, 0.35f, 0.0f, 1.0f) * 18.0f / 1200.0f);

    float amplitudeSum = 0.0f;
    for (std::size_t index = 0; index < modes_.size(); ++index) {
        auto& modeState = modes_[index];
        const auto partial = static_cast<float>(index + 1);
        float weight = 0.0f;

        switch (instrument_) {
            case AncientInstrument::aulos: {
                const auto pipeIndex = index < 6 ? index : index - 6;
                const auto pipeHarmonic = static_cast<float>(pipeIndex + 1);
                modeState.ratio = pipeHarmonic
                                  * (index < 6 ? 1.0f : detuneRatio);
                weight = aulosWeights[index]
                         * std::exp(-pipeHarmonic * (0.22f + 0.52f * (1.0f - safeBrightness)));
                break;
            }
            case AncientInstrument::lyre:
                modeState.ratio = lyreRatios[index]
                                  + partial * partial * 0.0012f * safeBrightness;
                weight = lyreWeights[index]
                         * std::exp(-partial * (0.12f + 0.34f * (1.0f - safeBrightness)));
                break;
            case AncientInstrument::bronzeGong:
                modeState.ratio = gongRatios[index];
                weight = gongWeights[index]
                         * std::exp(-partial * 0.065f * (1.0f - safeBrightness));
                break;
            case AncientInstrument::foundMetal:
                modeState.ratio = foundMetalRatios[index];
                weight = foundMetalWeights[index]
                         * std::exp(-partial * 0.035f * (1.0f - safeBrightness));
                break;
            case AncientInstrument::off:
                break;
        }

        const auto partialDecaySeconds = [&] {
            switch (instrument_) {
                case AncientInstrument::aulos:
                    return 1.0e9f;
                case AncientInstrument::lyre:
                    return safeDecay / (1.0f + 0.2f * static_cast<float>(index));
                case AncientInstrument::bronzeGong:
                    return safeDecay / (1.0f + 0.11f * static_cast<float>(index));
                case AncientInstrument::foundMetal:
                    return safeDecay * 0.58f / (1.0f + 0.17f * static_cast<float>(index));
                case AncientInstrument::off:
                    return 0.1f;
            }
            return 0.1f;
        }();

        modeState.amplitude = weight;
        modeState.decayMultiplier = instrument_ == AncientInstrument::aulos
                                        ? 1.0f
                                        : std::exp(-1.0f / (safeRate * partialDecaySeconds));
        modeState.phaseSine = 0.0;
        modeState.phaseCosine = 1.0;
        amplitudeSum += weight;
    }

    const auto normalization = amplitudeSum > 0.0f
                                   ? 0.82f / amplitudeSum
                                   : 0.0f;
    for (auto& modeState : modes_)
        modeState.amplitude *= normalization;

    const auto transientSeconds = instrument_ == AncientInstrument::foundMetal ? 0.26f : 0.035f;
    noiseEnvelope_ = instrument_ == AncientInstrument::aulos
                         ? 0.0f
                         : air_ * (instrument_ == AncientInstrument::lyre ? 0.04f : 0.1f);
    noiseDecayMultiplier_ = std::exp(-1.0f / (safeRate * transientSeconds));
    noiseLowpass_ = 0.0f;
    noiseState_ = noiseSeed == 0 ? 0x6d2b79f5u : noiseSeed;
    samplesUntilRetune_ = 0;
    updateRotations(frequency, safeRate);
    samplesUntilRetune_ = 16;
    active_ = true;
}

void AncientInstrumentVoice::updateRotations(float frequency, float sampleRate) noexcept {
    const auto safeFrequency = static_cast<double>(frequency);
    const auto safeRate = static_cast<double>(sampleRate);
    for (auto& modeState : modes_) {
        const auto partialFrequency = std::min(safeFrequency * modeState.ratio,
                                               safeRate * 0.45);
        const auto phaseStep = twoPi * partialFrequency / safeRate;
        modeState.rotationSine = std::sin(phaseStep);
        modeState.rotationCosine = std::cos(phaseStep);
    }
}

float AncientInstrumentVoice::nextNoise() noexcept {
    noiseState_ ^= noiseState_ << 13;
    noiseState_ ^= noiseState_ >> 17;
    noiseState_ ^= noiseState_ << 5;
    return static_cast<float>(noiseState_ >> 8) / 8388607.5f - 1.0f;
}

float AncientInstrumentVoice::nextSample(float frequency, float sampleRate) noexcept {
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
    for (auto& modeState : modes_) {
        if (modeState.amplitude <= 1.0e-7f)
            continue;

        output += static_cast<float>(modeState.phaseSine) * modeState.amplitude;
        const auto nextSine = modeState.phaseSine * modeState.rotationCosine
                              + modeState.phaseCosine * modeState.rotationSine;
        const auto nextCosine = modeState.phaseCosine * modeState.rotationCosine
                                - modeState.phaseSine * modeState.rotationSine;
        modeState.phaseSine = nextSine;
        modeState.phaseCosine = nextCosine;
        modeState.amplitude *= modeState.decayMultiplier;
        remainingEnergy += modeState.amplitude;
    }

    const auto whiteNoise = nextNoise();
    if (instrument_ == AncientInstrument::aulos) {
        noiseLowpass_ += 0.035f * (whiteNoise - noiseLowpass_);
        output += (whiteNoise - noiseLowpass_) * air_ * 0.11f;
    } else if (noiseEnvelope_ > 1.0e-7f) {
        output += whiteNoise * noiseEnvelope_;
        noiseEnvelope_ *= noiseDecayMultiplier_;
    }

    if (instrument_ != AncientInstrument::aulos
        && remainingEnergy < 1.0e-5f && noiseEnvelope_ < 1.0e-7f)
        active_ = false;

    return std::isfinite(output) ? std::clamp(output, -1.0f, 1.0f) : 0.0f;
}

void AncientInstrumentVoice::reset() noexcept {
    for (auto& modeState : modes_)
        modeState.amplitude = 0.0f;
    instrument_ = AncientInstrument::off;
    samplesUntilRetune_ = 0;
    air_ = 0.0f;
    noiseEnvelope_ = 0.0f;
    noiseDecayMultiplier_ = 0.0f;
    noiseLowpass_ = 0.0f;
    active_ = false;
}

} // namespace pas1::dsp
