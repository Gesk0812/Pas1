#include "EffectsChain.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr std::array<double, 4> reverbDelaySeconds{0.0297, 0.0371, 0.0411, 0.0437};
}

void EffectsChain::prepare(double sampleRate) {
    if (std::isfinite(sampleRate) && sampleRate > 0.0)
        sampleRate_ = sampleRate;

    const auto chorusSize = static_cast<std::size_t>(std::max(8.0, std::ceil(sampleRate_ * 0.06) + 4.0));
    const auto delaySize = static_cast<std::size_t>(std::max(8.0, std::ceil(sampleRate_ * 2.5) + 4.0));
    for (int channel = 0; channel < 2; ++channel) {
        chorusBuffers_[static_cast<std::size_t>(channel)].assign(chorusSize, 0.0f);
        delayBuffers_[static_cast<std::size_t>(channel)].assign(delaySize, 0.0f);
        for (std::size_t comb = 0; comb < reverbDelaySeconds.size(); ++comb) {
            const auto stereoOffset = channel == 0 ? 0.0 : 0.0013;
            const auto length = static_cast<std::size_t>(
                std::max(8.0, std::round(sampleRate_ * (reverbDelaySeconds[comb] + stereoOffset))));
            reverbCombs_[static_cast<std::size_t>(channel)][comb].assign(length, 0.0f);
        }
    }
    eqLowPassCoefficient_ = static_cast<float>(1.0 - std::exp(-2.0 * pi * 400.0 / sampleRate_));
    reset();
}

void EffectsChain::reset() noexcept {
    chorusRatePhase_ = 0.0f;
    phaserRatePhase_ = 0.0f;
    chorusWritePositions_.fill(0);
    delayWritePositions_.fill(0);
    reverbPositions_ = {};
    reverbDampingState_ = {};
    phaserState_ = {};
    eqLowState_ = {};
    for (auto& buffer : chorusBuffers_)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    for (auto& buffer : delayBuffers_)
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    for (auto& channel : reverbCombs_)
        for (auto& comb : channel)
            std::fill(comb.begin(), comb.end(), 0.0f);
}

float EffectsChain::readDelay(const std::vector<float>& buffer, std::size_t writePosition,
                              float delaySamples) noexcept {
    if (buffer.empty())
        return 0.0f;

    const auto size = static_cast<double>(buffer.size());
    auto readPosition = static_cast<double>(writePosition) - std::max(1.0f, delaySamples);
    while (readPosition < 0.0)
        readPosition += size;
    while (readPosition >= size)
        readPosition -= size;
    const auto index = static_cast<std::size_t>(readPosition);
    const auto nextIndex = (index + 1) % buffer.size();
    const auto fraction = static_cast<float>(readPosition - static_cast<double>(index));
    return buffer[index] * (1.0f - fraction) + buffer[nextIndex] * fraction;
}

float EffectsChain::blend(float dry, float wet, bool enabled, float mix) noexcept {
    const auto amount = enabled && std::isfinite(mix) ? std::clamp(mix, 0.0f, 1.0f) : 0.0f;
    return dry + (wet - dry) * amount;
}

void EffectsChain::process(float* const* channels, int numChannels, int numSamples,
                           const EffectsParameters& parameters) noexcept {
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    const auto activeChannels = std::min(numChannels, 2);
    const auto chorusRate = std::clamp(parameters.chorusRateHz, 0.01f, 8.0f);
    const auto chorusDepth = std::clamp(parameters.chorusDepth, 0.0f, 1.0f);
    const auto phaserRate = std::clamp(parameters.phaserRateHz, 0.01f, 8.0f);
    const auto phaserDepth = std::clamp(parameters.phaserDepth, 0.0f, 1.0f);
    const auto delayFeedback = std::clamp(parameters.delayFeedback, 0.0f, 0.92f);
    const auto delayTime = std::clamp(parameters.delayTimeSeconds, 0.01f, 2.45f);
    const auto reverbFeedback = 0.55f + std::clamp(parameters.reverbDecay, 0.0f, 1.0f) * 0.4f;
    const auto reverbDamping = 0.02f + std::clamp(parameters.reverbDamping, 0.0f, 1.0f) * 0.9f;
    const auto lowGain = std::pow(10.0f, std::clamp(parameters.eqLowGainDb, -12.0f, 12.0f) / 20.0f);
    const auto highGain = std::pow(10.0f, std::clamp(parameters.eqHighGainDb, -12.0f, 12.0f) / 20.0f);
    const auto chorusCenterDelay = static_cast<float>(sampleRate_ * 0.022);
    const auto chorusDepthSamples = static_cast<float>(sampleRate_ * 0.008) * chorusDepth;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        const auto chorusLfo = std::sin(2.0 * pi * chorusRatePhase_);
        const auto phaserLfo = std::sin(2.0 * pi * phaserRatePhase_);

        for (int channel = 0; channel < activeChannels; ++channel) {
            if (channels[channel] == nullptr)
                continue;
            auto sample = channels[channel][sampleIndex];
            if (!std::isfinite(sample))
                sample = 0.0f;

            const auto drive = 1.0f + std::clamp(parameters.distortionDrive, 0.0f, 1.0f) * 19.0f;
            const auto distorted = std::tanh(sample * drive) / std::tanh(drive);
            sample = blend(sample, distorted, parameters.distortionEnabled, parameters.distortionMix);

            auto& chorusBuffer = chorusBuffers_[static_cast<std::size_t>(channel)];
            auto& chorusWrite = chorusWritePositions_[static_cast<std::size_t>(channel)];
            if (!chorusBuffer.empty()) {
                chorusBuffer[chorusWrite] = sample;
                const auto stereoPhase = chorusLfo * (channel == 0 ? 1.0f : -1.0f);
                const auto delayed = readDelay(chorusBuffer, chorusWrite,
                                               chorusCenterDelay + stereoPhase * chorusDepthSamples);
                sample = blend(sample, delayed, parameters.chorusEnabled, parameters.chorusMix);
                chorusWrite = (chorusWrite + 1) % chorusBuffer.size();
            }

            const auto phaserFrequency = 300.0f + (1.0f + phaserLfo) * 1700.0f * phaserDepth;
            const auto normalizedFrequency = static_cast<float>(
                std::clamp(phaserFrequency / sampleRate_, 0.001, 0.45));
            const auto coefficient = (1.0f - normalizedFrequency) / (1.0f + normalizedFrequency);
            auto phaserWet = sample;
            for (std::size_t stage = 0; stage < 4; ++stage) {
                auto& state = phaserState_[static_cast<std::size_t>(channel)][stage];
                const auto output = -coefficient * phaserWet + state;
                state = phaserWet + coefficient * output;
                phaserWet = output;
            }
            sample = blend(sample, phaserWet, parameters.phaserEnabled, parameters.phaserMix);

            auto& delayBuffer = delayBuffers_[static_cast<std::size_t>(channel)];
            auto& delayWrite = delayWritePositions_[static_cast<std::size_t>(channel)];
            if (!delayBuffer.empty()) {
                const auto delayed = readDelay(delayBuffer, delayWrite,
                                               static_cast<float>(sampleRate_ * delayTime));
                delayBuffer[delayWrite] = sample + delayed * delayFeedback;
                sample = blend(sample, delayed, parameters.delayEnabled, parameters.delayMix);
                delayWrite = (delayWrite + 1) % delayBuffer.size();
            }

            float reverbWet = 0.0f;
            for (std::size_t combIndex = 0; combIndex < reverbDelaySeconds.size(); ++combIndex) {
                auto& comb = reverbCombs_[static_cast<std::size_t>(channel)][combIndex];
                if (comb.empty())
                    continue;
                auto& position = reverbPositions_[static_cast<std::size_t>(channel)][combIndex];
                auto& dampingState = reverbDampingState_[static_cast<std::size_t>(channel)][combIndex];
                const auto delayed = comb[position];
                dampingState += (delayed - dampingState) * (1.0f - reverbDamping);
                comb[position] = sample + dampingState * reverbFeedback;
                reverbWet += delayed * 0.25f;
                position = (position + 1) % comb.size();
            }
            sample = blend(sample, reverbWet, parameters.reverbEnabled, parameters.reverbMix);

            auto& lowState = eqLowState_[static_cast<std::size_t>(channel)];
            lowState += eqLowPassCoefficient_ * (sample - lowState);
            const auto equalized = lowState * lowGain + (sample - lowState) * highGain;
            sample = blend(sample, equalized, parameters.eqEnabled, parameters.eqMix);
            channels[channel][sampleIndex] = std::isfinite(sample) ? sample : 0.0f;
        }
        if (activeChannels == 2 && channels[0] != nullptr && channels[1] != nullptr) {
            const auto left = channels[0][sampleIndex];
            const auto right = channels[1][sampleIndex];
            const auto middle = 0.5f * (left + right);
            const auto side = 0.5f * (left - right)
                              * std::clamp(parameters.stereoWidth, 0.0f, 2.0f);
            channels[0][sampleIndex] = middle + side;
            channels[1][sampleIndex] = middle - side;
        }

        chorusRatePhase_ += chorusRate / static_cast<float>(sampleRate_);
        phaserRatePhase_ += phaserRate / static_cast<float>(sampleRate_);
        if (chorusRatePhase_ >= 1.0f)
            chorusRatePhase_ -= std::floor(chorusRatePhase_);
        if (phaserRatePhase_ >= 1.0f)
            phaserRatePhase_ -= std::floor(phaserRatePhase_);
    }
}

float EffectsChain::limitSample(float input, bool enabled, float threshold) noexcept {
    if (!std::isfinite(input))
        return 0.0f;
    if (!enabled)
        return std::clamp(input, -1.0f, 1.0f);

    threshold = std::clamp(std::isfinite(threshold) ? threshold : 0.92f, 0.1f, 0.999f);
    const auto magnitude = std::abs(input);
    if (magnitude <= threshold)
        return input;

    const auto kneeWidth = 1.0f - threshold;
    const auto limited = threshold + kneeWidth
                         * std::tanh((magnitude - threshold) / kneeWidth);
    return std::copysign(std::min(limited, 1.0f), input);
}

} // namespace pas1::dsp
