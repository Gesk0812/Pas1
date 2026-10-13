#pragma once

#include <algorithm>

namespace pas1::dsp::macro {

inline float normalizePercent(float percent) noexcept {
    return std::clamp(percent / 100.0f, 0.0f, 1.0f);
}

inline float bodyOffset(float normalizedBody) noexcept {
    return std::clamp(normalizedBody, 0.0f, 1.0f) - 0.5f;
}

inline float bodySubLevel(float baseLevel, float normalizedBody) noexcept {
    return std::clamp(baseLevel + bodyOffset(normalizedBody) * 0.48f, 0.0f, 1.0f);
}

inline float bodyOscillatorLevel(float baseLevel, float normalizedBody) noexcept {
    return std::clamp(baseLevel + bodyOffset(normalizedBody) * 0.20f, 0.0f, 1.0f);
}

inline float bodyFilterOctaves(float normalizedBody) noexcept {
    return -bodyOffset(normalizedBody) * 0.6f;
}

inline float brightnessFilterOctaves(float normalizedBrightness) noexcept {
    return (std::clamp(normalizedBrightness, 0.0f, 1.0f) - 0.5f) * 3.0f;
}

inline float brightnessWavetableOffset(float normalizedBrightness) noexcept {
    return (std::clamp(normalizedBrightness, 0.0f, 1.0f) - 0.5f) * 0.36f;
}

inline float movementScale(float normalizedMovement) noexcept {
    return 0.5f + std::clamp(normalizedMovement, 0.0f, 1.0f);
}

} // namespace pas1::dsp::macro
