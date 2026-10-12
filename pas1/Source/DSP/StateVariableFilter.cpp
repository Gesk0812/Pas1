#include "StateVariableFilter.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {
namespace {
constexpr double pi = 3.14159265358979323846;
}

void StateVariableFilter::prepare(double sampleRate) noexcept {
    if (std::isfinite(sampleRate) && sampleRate > 0.0)
        sampleRate_ = sampleRate;
    reset();
    setParameters(12000.0f, 0.70710678f, FilterType::lowPass);
}

void StateVariableFilter::reset() noexcept {
    ic1eq_ = 0.0f;
    ic2eq_ = 0.0f;
}

void StateVariableFilter::setParameters(float cutoffHz, float resonance, FilterType type) noexcept {
    type_ = type;
    if (!std::isfinite(cutoffHz))
        cutoffHz = 1000.0f;
    if (!std::isfinite(resonance))
        resonance = 0.70710678f;

    const auto maximumCutoff = static_cast<float>(std::min(20000.0, sampleRate_ * 0.45));
    cutoffHz = std::clamp(cutoffHz, 10.0f, std::max(10.0f, maximumCutoff));
    resonance = std::clamp(resonance, 0.1f, 0.99f);

    const auto q = 0.5 + static_cast<double>(resonance) * 11.5;
    const auto g = std::tan(pi * static_cast<double>(cutoffHz) / sampleRate_);
    const auto k = 1.0 / q;
    const auto a1 = 1.0 / (1.0 + g * (g + k));

    g_ = static_cast<float>(g);
    k_ = static_cast<float>(k);
    a1_ = static_cast<float>(a1);
    a2_ = static_cast<float>(g * a1);
    a3_ = static_cast<float>(g * a2_);
}

float StateVariableFilter::processSample(float input) noexcept {
    if (!std::isfinite(input)) {
        reset();
        return 0.0f;
    }

    const auto v3 = input - ic2eq_;
    const auto v1 = a1_ * ic1eq_ + a2_ * v3;
    const auto v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
    ic1eq_ = 2.0f * v1 - ic1eq_;
    ic2eq_ = 2.0f * v2 - ic2eq_;

    float output = v2;
    switch (type_) {
        case FilterType::lowPass:
            output = v2;
            break;
        case FilterType::highPass:
            output = input - k_ * v1 - v2;
            break;
        case FilterType::bandPass:
            output = v1;
            break;
        case FilterType::notch:
            output = input - k_ * v1;
            break;
    }

    if (!std::isfinite(ic1eq_) || !std::isfinite(ic2eq_) || !std::isfinite(output)) {
        reset();
        return 0.0f;
    }
    return std::clamp(output, -16.0f, 16.0f);
}

} // namespace pas1::dsp
