#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "DSP/ModulationMatrix.h"
#include "UI/Utf8Text.h"

#include <array>
#include <memory>

class ModulationPanel final : public juce::Component {
public:
    explicit ModulationPanel(juce::AudioProcessorValueTreeState& state);
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    class RouteRow final : public juce::Component {
    public:
        RouteRow(juce::AudioProcessorValueTreeState& state, std::size_t routeIndex);
        void paint(juce::Graphics& graphics) override;
        void resized() override;

    private:
        std::size_t routeIndex_;
        juce::ComboBox source;
        juce::ComboBox destination;
        juce::Slider amount;
        juce::ComboBox polarity;
        juce::ToggleButton enabled{pas1::ui::utf8String("ВКЛ")};
        std::unique_ptr<ComboBoxAttachment> sourceAttachment;
        std::unique_ptr<ComboBoxAttachment> destinationAttachment;
        std::unique_ptr<SliderAttachment> amountAttachment;
        std::unique_ptr<ComboBoxAttachment> polarityAttachment;
        std::unique_ptr<ButtonAttachment> enabledAttachment;
    };

    juce::Viewport viewport;
    juce::Component rowsContent;
    std::array<juce::Label, 6> columnLabels;
    std::array<std::unique_ptr<RouteRow>, pas1::dsp::ModulationMatrix::routeCount> rows;
};
