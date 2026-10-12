#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"
#include "UI/Utf8Text.h"

#include <array>

class MorphPad final : public juce::Component, private juce::Timer {
public:
    explicit MorphPad(Pas1AudioProcessor& processor);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void updatePosition(juce::Point<float> position);
    juce::Rectangle<float> padArea() const;

    Pas1AudioProcessor& processor_;
    juce::RangedAudioParameter* xParameter = nullptr;
    juce::RangedAudioParameter* yParameter = nullptr;
    bool dragging = false;
};

class MorphPanel final : public juce::Component {
public:
    explicit MorphPanel(Pas1AudioProcessor& processor);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Pas1AudioProcessor& processor_;
    MorphPad pad;
    std::array<juce::TextButton, 4> captureButtons{};
    std::array<juce::ToggleButton, pas1::dsp::MorphMatrix::parameterCount> locks{};
    juce::TextButton factoryButton{pas1::ui::utf8String("ЗАВОДСКИЕ УГЛЫ")};
    juce::TextButton randomizeButton{pas1::ui::utf8String("СЛУЧАЙНЫЕ ЗВУКИ")};
    juce::Label status;
};
