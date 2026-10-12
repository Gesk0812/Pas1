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
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    enum class DragHandle { none, filterResponse, attack, decaySustain, release };
    struct EnvelopeLayout {
        juce::Rectangle<float> bounds;
        float attackX = 0.0f;
        float decayX = 0.0f;
        float sustainY = 0.0f;
        float releaseX = 0.0f;
    };

    static constexpr std::size_t responsePointCount = 192;
    static constexpr float envelopeHoldFraction = 0.22f;
    juce::Rectangle<float> responsePlotBounds() const noexcept;
    EnvelopeLayout envelopeLayout() const noexcept;
    void calculateResponse() noexcept;
    void updateFromMouse(juce::Point<float> position);
    void beginParameterGesture(const char* id);
    void endParameterGestures();
    void setParameterValue(const char* id, float value);

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
    DragHandle activeHandle_ = DragHandle::none;
    std::array<juce::RangedAudioParameter*, 2> gestureParameters_{};
    std::size_t gestureParameterCount_ = 0;
};
