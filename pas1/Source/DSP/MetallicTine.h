#pragma once

#include <array>

namespace pas1::dsp {

// A compact, allocation-free bank of decaying modes for struck metal tines.
class MetallicTine {
public:
    void trigger(float frequency, float sampleRate, float brightness, float decaySeconds,
                 float resonance, float inharmonicity, float strike) noexcept;
    float nextSample(float frequency, float sampleRate) noexcept;
    void reset() noexcept;

private:
    struct Mode {
        double phaseSine = 0.0;
        double phaseCosine = 1.0;
        double rotationSine = 0.0;
        double rotationCosine = 1.0;
        float ratio = 1.0f;
        float amplitude = 0.0f;
        float decayMultiplier = 0.0f;
    };

    void updateRotations(float frequency, float sampleRate) noexcept;

    static constexpr std::size_t modeCount = 6;
    std::array<Mode, modeCount> modes_{};
    int samplesUntilRetune_ = 0;
    bool active_ = false;
};

} // namespace pas1::dsp
