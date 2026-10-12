#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>
#include <vector>

struct ParameterPageGroup {
    juce::String title;
    std::vector<juce::String> parameterIds;
};

struct ParameterPageTheme {
    juce::Colour background{0xff302015};
    juce::Colour rowBackground{0xff3a271b};
    juce::Colour accent{0xffd8b36a};
    juce::Colour text{0xfff4e8cf};
    juce::Colour muted{0xffbba483};
    juce::Colour border{0xff745332};
};

class ParameterPage final : public juce::Component {
public:
    ParameterPage(juce::AudioProcessorValueTreeState& state,
                  juce::String title,
                  std::vector<ParameterPageGroup> groups,
                  ParameterPageTheme theme = {},
                  juce::Component* headerComponent = nullptr,
                  int headerComponentHeight = 0);
    ~ParameterPage() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class Content;

    juce::String title_;
    ParameterPageTheme theme_;
    juce::Component* headerComponent_ = nullptr;
    int headerComponentHeight_ = 0;
    juce::Viewport viewport_;
    std::unique_ptr<Content> content_;
};
