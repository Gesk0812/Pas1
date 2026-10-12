#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "DSP/SoundDNA.h"
#include "PluginProcessor.h"

#include <optional>
#include <vector>

class SoundDNAPanel final : public juce::Component {
public:
    explicit SoundDNAPanel(Pas1AudioProcessor& processor);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using ParameterList = std::vector<pas1::generation::ParameterValue>;
    ParameterList captureParameters(bool useDefaults = false) const;
    ParameterList builtInParameters(int index) const;
    pas1::dsp::SoundDNAProfile builtInProfile(int index) const;
    void analyzeCurrent();
    void findSimilar();
    void compareToBuiltIn();
    void blendSelected();
    void exploreSoundSpace();
    void generateFromDNA();
    void keepCurrent();
    void revert();
    void loadSelectedMatch();
    void setStatus(const juce::String& message);
    float getBlendAmount() const;
    void drawProfile(juce::Graphics& graphics, const juce::Rectangle<int>& area) const;

    Pas1AudioProcessor& processor_;
    juce::ComboBox presetA;
    juce::ComboBox presetB;
    juce::ComboBox similarMatches;
    juce::Slider blendAmount;
    juce::Label presetALabel, presetBLabel, blendLabel, status;
    juce::TextButton analyzeButton{"ANALYZE CURRENT"};
    juce::TextButton similarButton{"FIND SIMILAR"};
    juce::TextButton compareButton{"COMPARE"};
    juce::TextButton blendButton{"BLEND A + B"};
    juce::TextButton exploreButton{"EXPLORE SPACE"};
    juce::TextButton generateButton{"GENERATE FROM DNA"};
    juce::TextButton loadMatchButton{"LOAD MATCH"};
    juce::TextButton keepButton{"KEEP"};
    juce::TextButton revertButton{"REVERT"};
    pas1::dsp::SoundDNAProfile currentProfile;
    pas1::dsp::SoundDNAProfile displayProfile;
    std::optional<pas1::dsp::SoundDNAProfile> pendingProfile;
    std::vector<int> similarPresetIndices;
    juce::ValueTree originalState;
    bool previewOpen = false;
};
