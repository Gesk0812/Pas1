#pragma once

#include <array>
#include <vector>

namespace pas1::dsp {

struct EffectsParameters {
    bool distortionEnabled = true;
    float distortionMix = 0.0f;
    float distortionDrive = 0.25f;
    bool chorusEnabled = false;
    float chorusMix = 0.0f;
    float chorusRateHz = 0.8f;
    float chorusDepth = 0.5f;
    bool phaserEnabled = false;
    float phaserMix = 0.0f;
    float phaserRateHz = 0.25f;
    float phaserDepth = 0.5f;
    bool delayEnabled = false;
    float delayMix = 0.0f;
    float delayTimeSeconds = 0.35f;
    float delayFeedback = 0.25f;
    bool reverbEnabled = false;
    float reverbMix = 0.0f;
    float reverbDecay = 0.5f;
    float reverbDamping = 0.5f;
    float stereoWidth = 1.0f;
    bool eqEnabled = false;
    float eqMix = 1.0f;
    float eqLowGainDb = 0.0f;
    float eqHighGainDb = 0.0f;
    bool limiterEnabled = true;
    float limiterThreshold = 0.92f;
};

class EffectsChain {
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples,
                 const EffectsParameters& parameters) noexcept;
    static float limitSample(float input, bool enabled, float threshold) noexcept;

private:
    static float readDelay(const std::vector<float>& buffer, std::size_t writePosition,
                           float delaySamples) noexcept;
    static float blend(float dry, float wet, bool enabled, float mix) noexcept;

    double sampleRate_ = 44100.0;
    float chorusRatePhase_ = 0.0f;
    float phaserRatePhase_ = 0.0f;
    float eqLowPassCoefficient_ = 0.0f;

    std::array<std::vector<float>, 2> chorusBuffers_{};
    std::array<std::size_t, 2> chorusWritePositions_{};
    std::array<std::vector<float>, 2> delayBuffers_{};
    std::array<std::size_t, 2> delayWritePositions_{};
    std::array<std::array<std::vector<float>, 4>, 2> reverbCombs_{};
    std::array<std::array<std::size_t, 4>, 2> reverbPositions_{};
    std::array<std::array<float, 4>, 2> reverbDampingState_{};
    std::array<std::array<float, 4>, 2> phaserState_{};
    std::array<float, 2> eqLowState_{};
};

} // namespace pas1::dsp
