#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

#include <array>

class OscillatorWaveformGraph final : public juce::Component {
public:
    explicit OscillatorWaveformGraph(Pas1AudioProcessor& processor) : processor_(processor) {}

    void update();
    void paint(juce::Graphics&) override;

private:
    Pas1AudioProcessor& processor_;
};

class FilterGraphs final : public juce::Component {
public:
    explicit FilterGraphs(Pas1AudioProcessor& processor) : processor_(processor) {}

    void update();
    void paint(juce::Graphics&) override;

private:
    static constexpr std::size_t responsePointCount = 192;
    void calculateResponse() noexcept;

    Pas1AudioProcessor& processor_;
    std::array<float, responsePointCount> responseDb_{};
    float cutoffHz_ = -1.0f;
    float resonance_ = -1.0f;
    float sampleRate_ = -1.0f;
    float envelopeAmount_ = 0.0f;
    float attack_ = 0.01f;
    float decay_ = 0.2f;
    float sustain_ = 0.7f;
    float release_ = 0.2f;
    int filterType_ = -1;
};
