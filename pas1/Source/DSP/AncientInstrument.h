#pragma once

#include <array>
#include <cstdint>

namespace pas1::dsp {

enum class AncientInstrument : int {
    off = 0,
    aulos,
    lyre,
    bronzeGong,
    foundMetal
};

// Small modal voices inspired by ancient reed, string and bronze instruments.
// The synth models these timbres algorithmically; it does not use soundtrack samples.
class AncientInstrumentVoice {
public:
    void trigger(AncientInstrument instrument, float frequency, float sampleRate,
                 float brightness, float decaySeconds, float air,
                 float pipeDetune, std::uint32_t noiseSeed) noexcept;
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
        float decayMultiplier = 1.0f;
    };

    void updateRotations(float frequency, float sampleRate) noexcept;
    float nextNoise() noexcept;

    static constexpr std::size_t modeCount = 10;
    std::array<Mode, modeCount> modes_{};
    AncientInstrument instrument_ = AncientInstrument::off;
    int samplesUntilRetune_ = 0;
    std::uint32_t noiseState_ = 0x6d2b79f5u;
    float air_ = 0.0f;
    float noiseEnvelope_ = 0.0f;
    float noiseDecayMultiplier_ = 0.0f;
    float noiseLowpass_ = 0.0f;
    bool active_ = false;
};

} // namespace pas1::dsp
