#pragma once

namespace pas1::dsp {

struct EnvelopeParameters {
    float attackSeconds = 0.01f;
    float decaySeconds = 0.2f;
    float sustainLevel = 0.75f;
    float releaseSeconds = 0.5f;
};

class AmpEnvelope {
public:
    void setSampleRate(double sampleRate) noexcept;
    void setParameters(EnvelopeParameters parameters) noexcept;
    void noteOn() noexcept;
    void noteOff() noexcept;
    float nextSample() noexcept;
    bool isActive() const noexcept;
    float level() const noexcept { return level_; }

private:
    enum class Stage { idle, attack, decay, sustain, release };

    static unsigned long long samplesFor(float seconds, double sampleRate) noexcept;
    float currentStageLevel() const noexcept;
    void enter(Stage stage) noexcept;

    EnvelopeParameters parameters_;
    double sampleRate_ = 44100.0;
    Stage stage_ = Stage::idle;
    unsigned long long position_ = 0;
    unsigned long long duration_ = 0;
    float level_ = 0.0f;
    float releaseStartLevel_ = 0.0f;
};

} // namespace pas1::dsp
