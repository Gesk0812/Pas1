#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>
#include <vector>

struct ParameterPageGroup {
    juce::String title;
    std::vector<juce::String> parameterIds;
};

class ParameterPage final : public juce::Component {
public:
    ParameterPage(juce::AudioProcessorValueTreeState& state,
                  juce::String title,
                  std::vector<ParameterPageGroup> groups);
    ~ParameterPage() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class Content;

    juce::String title_;
    juce::Viewport viewport_;
    std::unique_ptr<Content> content_;
};
