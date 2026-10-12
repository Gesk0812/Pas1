#include "Portamento.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {

float Portamento::sanitizeFrequency(float frequencyHz) noexcept {
    return std::isfinite(frequencyHz)
               ? std::clamp(frequencyHz, 1.0f, 20000.0f)
               : 440.0f;
}

void Portamento::reset(float frequencyHz) noexcept {
    startFrequencyHz_ = sanitizeFrequency(frequencyHz);
    targetFrequencyHz_ = startFrequencyHz_;
    currentFrequencyHz_ = startFrequencyHz_;
    elapsedSamples_ = 0;
    durationSamples_ = 0;
}

void Portamento::glideTo(float targetFrequencyHz, std::uint64_t durationSamples) noexcept {
    targetFrequencyHz_ = sanitizeFrequency(targetFrequencyHz);
    if (durationSamples == 0 || !std::isfinite(currentFrequencyHz_)
        || currentFrequencyHz_ <= 0.0f
        || std::abs(targetFrequencyHz_ - currentFrequencyHz_) < 0.0001f) {
        reset(targetFrequencyHz_);
        return;
    }
    startFrequencyHz_ = sanitizeFrequency(currentFrequencyHz_);
    elapsedSamples_ = 0;
    durationSamples_ = durationSamples;
}

float Portamento::nextSample() noexcept {
    if (elapsedSamples_ >= durationSamples_)
        return currentFrequencyHz_ = targetFrequencyHz_;

    ++elapsedSamples_;
    const auto progress = static_cast<float>(elapsedSamples_)
                          / static_cast<float>(durationSamples_);
    currentFrequencyHz_ = startFrequencyHz_
                          * std::pow(targetFrequencyHz_ / startFrequencyHz_, progress);
    if (elapsedSamples_ >= durationSamples_)
        currentFrequencyHz_ = targetFrequencyHz_;
    return currentFrequencyHz_;
}

} // namespace pas1::dsp
