#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "DSP/SoundDNA.h"
#include "PluginProcessor.h"
#include "UI/Utf8Text.h"

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
    juce::TextButton analyzeButton{pas1::ui::utf8String("АНАЛИЗ")};
    juce::TextButton similarButton{pas1::ui::utf8String("ПОХОЖИЕ")};
    juce::TextButton compareButton{pas1::ui::utf8String("СРАВНИТЬ")};
    juce::TextButton blendButton{pas1::ui::utf8String("СМЕШАТЬ A + B")};
    juce::TextButton exploreButton{pas1::ui::utf8String("ИССЛЕДОВАТЬ")};
    juce::TextButton generateButton{pas1::ui::utf8String("СОЗДАТЬ ЗВУК")};
    juce::TextButton loadMatchButton{pas1::ui::utf8String("ЗАГРУЗИТЬ")};
    juce::TextButton keepButton{pas1::ui::utf8String("ОСТАВИТЬ")};
    juce::TextButton revertButton{pas1::ui::utf8String("ОТМЕНИТЬ")};
    pas1::dsp::SoundDNAProfile currentProfile;
    pas1::dsp::SoundDNAProfile displayProfile;
    std::optional<pas1::dsp::SoundDNAProfile> pendingProfile;
    std::vector<int> similarPresetIndices;
    juce::ValueTree originalState;
    bool previewOpen = false;
};
