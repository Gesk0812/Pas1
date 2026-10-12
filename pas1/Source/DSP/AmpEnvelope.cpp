#include "AmpEnvelope.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {

unsigned long long AmpEnvelope::samplesFor(float seconds, double sampleRate) noexcept {
    if (!std::isfinite(seconds) || seconds <= 0.0f)
        return 0;

    const auto samples = static_cast<double>(seconds) * sampleRate;
    return static_cast<unsigned long long>(std::llround(std::clamp(samples, 1.0, 1000000000.0)));
}

void AmpEnvelope::setSampleRate(double sampleRate) noexcept {
    if (std::isfinite(sampleRate) && sampleRate > 0.0)
        sampleRate_ = sampleRate;
}

float AmpEnvelope::currentStageLevel() const noexcept {
    if (duration_ == 0)
        return level_;

    const auto progress = std::clamp(static_cast<double>(position_) / static_cast<double>(duration_), 0.0, 1.0);
    switch (stage_) {
        case Stage::attack:
            return static_cast<float>(progress);
        case Stage::decay:
            return static_cast<float>(1.0 + (parameters_.sustainLevel - 1.0) * progress);
        case Stage::release:
            return static_cast<float>(releaseStartLevel_ * (1.0 - progress));
        case Stage::idle:
            return 0.0f;
        case Stage::sustain:
            return parameters_.sustainLevel;
    }
    return 0.0f;
}

void AmpEnvelope::setParameters(EnvelopeParameters parameters) noexcept {
    parameters.attackSeconds = std::isfinite(parameters.attackSeconds)
                                   ? std::clamp(parameters.attackSeconds, 0.0f, 30.0f)
                                   : 0.01f;
    parameters.decaySeconds = std::isfinite(parameters.decaySeconds)
                                  ? std::clamp(parameters.decaySeconds, 0.0f, 30.0f)
                                  : 0.2f;
    parameters.sustainLevel = std::isfinite(parameters.sustainLevel)
                                  ? std::clamp(parameters.sustainLevel, 0.0f, 1.0f)
                                  : 0.75f;
    parameters.releaseSeconds = std::isfinite(parameters.releaseSeconds)
                                    ? std::clamp(parameters.releaseSeconds, 0.0f, 30.0f)
                                    : 0.5f;

    const auto oldLevel = currentStageLevel();
    const auto oldDuration = duration_;
    parameters_ = parameters;

    unsigned long long newDuration = duration_;
    if (stage_ == Stage::attack)
        newDuration = samplesFor(parameters_.attackSeconds, sampleRate_);
    else if (stage_ == Stage::decay)
        newDuration = samplesFor(parameters_.decaySeconds, sampleRate_);
    else if (stage_ == Stage::release)
        newDuration = samplesFor(parameters_.releaseSeconds, sampleRate_);

    if (stage_ == Stage::release)
        releaseStartLevel_ = oldLevel;

    if (oldDuration > 0 && newDuration > 0)
        position_ = static_cast<unsigned long long>(
            (static_cast<double>(position_) / static_cast<double>(oldDuration)) * newDuration);
    else
        position_ = 0;

    duration_ = newDuration;
    level_ = oldLevel;
}

void AmpEnvelope::enter(Stage stage) noexcept {
    stage_ = stage;
    position_ = 0;
    switch (stage_) {
        case Stage::attack:
            duration_ = samplesFor(parameters_.attackSeconds, sampleRate_);
            break;
        case Stage::decay:
            duration_ = samplesFor(parameters_.decaySeconds, sampleRate_);
            break;
        case Stage::release:
            duration_ = samplesFor(parameters_.releaseSeconds, sampleRate_);
            break;
        case Stage::idle:
        case Stage::sustain:
            duration_ = 0;
            break;
    }
}

void AmpEnvelope::noteOn() noexcept {
    level_ = 0.0f;
    enter(Stage::attack);
    if (duration_ == 0) {
        level_ = 1.0f;
        enter(Stage::decay);
        if (duration_ == 0) {
            level_ = parameters_.sustainLevel;
            enter(Stage::sustain);
        }
    }
}

void AmpEnvelope::noteOff() noexcept {
    if (stage_ == Stage::idle || stage_ == Stage::release)
        return;

    level_ = currentStageLevel();
    releaseStartLevel_ = level_;
    enter(Stage::release);
    if (duration_ == 0) {
        level_ = 0.0f;
        enter(Stage::idle);
    }
}

float AmpEnvelope::nextSample() noexcept {
    switch (stage_) {
        case Stage::idle:
            level_ = 0.0f;
            return 0.0f;
        case Stage::sustain:
            level_ = parameters_.sustainLevel;
            return level_;
        case Stage::attack:
            if (duration_ == 0) {
                level_ = 1.0f;
                enter(Stage::decay);
                return nextSample();
            }
            level_ = static_cast<float>(static_cast<double>(position_) / static_cast<double>(duration_));
            if (++position_ >= duration_) {
                level_ = 1.0f;
                enter(Stage::decay);
            }
            return level_;
        case Stage::decay:
            if (duration_ == 0) {
                level_ = parameters_.sustainLevel;
                enter(Stage::sustain);
                return level_;
            }
            level_ = static_cast<float>(
                1.0 + (parameters_.sustainLevel - 1.0)
                          * (static_cast<double>(position_) / static_cast<double>(duration_)));
            if (++position_ >= duration_) {
                level_ = parameters_.sustainLevel;
                enter(Stage::sustain);
            }
            return level_;
        case Stage::release:
            if (duration_ == 0) {
                level_ = 0.0f;
                enter(Stage::idle);
                return level_;
            }
            level_ = static_cast<float>(
                releaseStartLevel_ * (1.0 - static_cast<double>(position_) / static_cast<double>(duration_)));
            if (++position_ >= duration_) {
                level_ = 0.0f;
                enter(Stage::idle);
            }
            return level_;
    }
    return 0.0f;
}

bool AmpEnvelope::isActive() const noexcept {
    return stage_ != Stage::idle;
}

} // namespace pas1::dsp
