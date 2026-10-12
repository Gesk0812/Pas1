#pragma once

namespace pas1::dsp {

enum class FilterType {
    lowPass = 0,
    highPass,
    bandPass,
    notch
};

class StateVariableFilter {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setParameters(float cutoffHz, float resonance, FilterType type) noexcept;
    float processSample(float input) noexcept;

private:
    double sampleRate_ = 44100.0;
    float g_ = 0.0f;
    float k_ = 1.41421356f;
    float a1_ = 1.0f;
    float a2_ = 0.0f;
    float a3_ = 0.0f;
    float ic1eq_ = 0.0f;
    float ic2eq_ = 0.0f;
    FilterType type_ = FilterType::lowPass;
};

} // namespace pas1::dsp
