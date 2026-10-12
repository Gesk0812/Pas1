#include "MorphMatrix.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {
namespace {
float finiteOr(float value, float fallback) noexcept {
    return std::isfinite(value) ? value : fallback;
}
}

MorphMatrix::MorphMatrix() noexcept {
    for (auto& corner : corners_)
        for (auto& value : corner)
            value.store(0.0f, std::memory_order_relaxed);
    for (auto& lock : locked_)
        lock.store(false, std::memory_order_relaxed);
}

void MorphMatrix::captureCorner(int corner, const Values& values) noexcept {
    if (corner < 0 || corner >= static_cast<int>(corners_.size()))
        return;
    if (!isEnabled()) {
        for (auto& initialCorner : corners_)
            for (std::size_t index = 0; index < parameterCount; ++index)
                initialCorner[index].store(finiteOr(values[index], 0.0f), std::memory_order_relaxed);
    }
    auto& destination = corners_[static_cast<std::size_t>(corner)];
    for (std::size_t index = 0; index < parameterCount; ++index)
        destination[index].store(finiteOr(values[index], 0.0f), std::memory_order_relaxed);
    enabled_.store(true, std::memory_order_release);
}

MorphMatrix::Values MorphMatrix::getCorner(int corner) const noexcept {
    Values result{};
    if (corner < 0 || corner >= static_cast<int>(corners_.size()))
        return result;
    const auto& source = corners_[static_cast<std::size_t>(corner)];
    for (std::size_t index = 0; index < parameterCount; ++index)
        result[index] = source[index].load(std::memory_order_relaxed);
    return result;
}

void MorphMatrix::setEnabled(bool enabled) noexcept {
    enabled_.store(enabled, std::memory_order_release);
}

bool MorphMatrix::isEnabled() const noexcept {
    return enabled_.load(std::memory_order_acquire);
}

void MorphMatrix::setLocked(std::size_t parameter, bool locked) noexcept {
    if (parameter < parameterCount)
        locked_[parameter].store(locked, std::memory_order_relaxed);
}

bool MorphMatrix::isLocked(std::size_t parameter) const noexcept {
    return parameter < parameterCount && locked_[parameter].load(std::memory_order_relaxed);
}

float MorphMatrix::interpolate(float a, float b, float c, float d, float x, float y,
                               bool logarithmic) noexcept {
    const auto weightA = (1.0f - x) * (1.0f - y);
    const auto weightB = x * (1.0f - y);
    const auto weightC = (1.0f - x) * y;
    const auto weightD = x * y;
    if (logarithmic) {
        constexpr float minimumPositive = 0.001f;
        const auto logValue = std::log(std::max(a, minimumPositive)) * weightA
                              + std::log(std::max(b, minimumPositive)) * weightB
                              + std::log(std::max(c, minimumPositive)) * weightC
                              + std::log(std::max(d, minimumPositive)) * weightD;
        return std::exp(logValue);
    }
    return a * weightA + b * weightB + c * weightC + d * weightD;
}

MorphMatrix::Values MorphMatrix::process(float x, float y, const Values& current,
                                         double sampleRate, int blockSize) noexcept {
    Values result{};
    if (!isEnabled()) {
        smoothedValues_ = current;
        smoothingInitialized_.store(false, std::memory_order_relaxed);
        return current;
    }

    x = std::clamp(finiteOr(x, 0.0f), 0.0f, 1.0f);
    y = std::clamp(finiteOr(y, 0.0f), 0.0f, 1.0f);
    const auto a = getCorner(0);
    const auto b = getCorner(1);
    const auto c = getCorner(2);
    const auto d = getCorner(3);
    if (!smoothingInitialized_.load(std::memory_order_relaxed)) {
        smoothedValues_ = current;
        smoothingInitialized_.store(true, std::memory_order_relaxed);
    }

    const auto safeRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto safeBlockSize = std::max(1, blockSize);
    const auto smoothing = static_cast<float>(
        1.0 - std::exp(-static_cast<double>(safeBlockSize) / (safeRate * 0.025)));
    for (std::size_t index = 0; index < parameterCount; ++index) {
        if (locked_[index].load(std::memory_order_relaxed)) {
            smoothedValues_[index] = finiteOr(current[index], 0.0f);
            result[index] = smoothedValues_[index];
            continue;
        }

        const auto logarithmic = index == static_cast<std::size_t>(MorphParameter::filterCutoff)
                                 || index == static_cast<std::size_t>(MorphParameter::attack)
                                 || index == static_cast<std::size_t>(MorphParameter::release);
        const auto target = interpolate(a[index], b[index], c[index], d[index], x, y, logarithmic);
        const auto safeTarget = finiteOr(target, current[index]);
        smoothedValues_[index] += (safeTarget - smoothedValues_[index]) * smoothing;
        result[index] = smoothedValues_[index];
    }
    return result;
}

std::uint32_t MorphMatrix::nextRandom(std::uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void MorphMatrix::randomizeCorners(std::uint32_t seed) noexcept {
    if (!isEnabled())
        return;
    auto randomState = seed == 0 ? 0x9e3779b9u : seed;
    for (int corner = 1; corner < 4; ++corner) {
        auto values = getCorner(corner);
        for (std::size_t index = 0; index < parameterCount; ++index) {
            if (locked_[index].load(std::memory_order_relaxed))
                continue;
            const auto random = static_cast<float>(nextRandom(randomState) >> 8) / 8388607.5f - 1.0f;
            const auto parameter = static_cast<MorphParameter>(index);
            switch (parameter) {
                case MorphParameter::filterCutoff:
                    values[index] *= std::exp2(random * 1.25f);
                    values[index] = std::clamp(values[index], 20.0f, 20000.0f);
                    break;
                case MorphParameter::attack:
                    values[index] *= std::exp2(random * 1.0f);
                    values[index] = std::clamp(values[index], 0.001f, 5.0f);
                    break;
                case MorphParameter::release:
                    values[index] *= std::exp2(random * 1.0f);
                    values[index] = std::clamp(values[index], 0.01f, 10.0f);
                    break;
                case MorphParameter::unisonDetune:
                    values[index] = std::clamp(values[index] + random * 18.0f, 0.0f, 50.0f);
                    break;
                case MorphParameter::stereoWidth:
                    values[index] = std::clamp(values[index] + random * 0.45f, 0.0f, 2.0f);
                    break;
                default:
                    values[index] = std::clamp(values[index] + random * 0.24f, 0.0f, 1.0f);
                    break;
            }
        }
        captureCorner(corner, values);
    }
}

} // namespace pas1::dsp
