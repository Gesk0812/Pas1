#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Generation/GeneratorPanel.h"
#include "Generation/MorphPanel.h"
#include "Generation/ModulationPanel.h"
#include "Generation/SoundDNAPanel.h"
#include "PluginProcessor.h"
#include "Presets/PresetsPanel.h"
#include "UI/ParameterPage.h"
#include "UI/SpectrumAnalyzer.h"

class Pas1AudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit Pas1AudioProcessorEditor(Pas1AudioProcessor&);
    ~Pas1AudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    enum class Page {
        home,
        oscillators,
        filters,
        modulation,
        morph,
        generator,
        soundDNA,
        effects,
        presets,
        settings,
        allParameters
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    void setCoreControlsVisible(bool visible);
    void showPage(Page page);
    void timerCallback() override;
    void savePreset();
    void showPresetError(const juce::String& message);

    Pas1AudioProcessor& processor_;
    std::unique_ptr<juce::GenericAudioProcessorEditor> engineEditor;
    juce::TextButton homeButton{"HOME"};
    juce::TextButton oscillatorButton{"OSC"};
    juce::TextButton filterButton{"FILTER"};
    juce::TextButton effectsButton{"FX"};
    juce::TextButton settingsButton{"SETTINGS"};
    juce::TextButton engineButton{"ALL PARAMETERS"};
    juce::TextButton presetButton{"PRESETS"};
    juce::TextButton savePresetButton{"SAVE AS"};
    juce::TextButton generatorButton{"GENERATOR"};
    juce::TextButton morphButton{"MORPH"};
    juce::TextButton soundDNAButton{"SOUND DNA"};
    juce::TextButton modulationButton{"MOD"};
    juce::TextButton resetDefaultsButton{"RESET PARAMETERS"};
    juce::TextButton storeSnapshotAButton{"STORE A"};
    juce::TextButton storeSnapshotBButton{"STORE B"};
    juce::TextButton compareSnapshotsButton{"A/B"};
    Page currentPage = Page::home;
    juce::MemoryBlock snapshotA;
    juce::MemoryBlock snapshotB;
    bool showingSnapshotB = false;
    std::unique_ptr<ParameterPage> oscillatorPage;
    std::unique_ptr<ParameterPage> filterPage;
    std::unique_ptr<ParameterPage> lfoPage;
    std::unique_ptr<ParameterPage> effectsPage;
    std::unique_ptr<ParameterPage> settingsPage;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    std::unique_ptr<GeneratorPanel> generatorPanel;
    std::unique_ptr<MorphPanel> morphPanel;
    std::unique_ptr<SoundDNAPanel> soundDNAPanel;
    std::unique_ptr<PresetsPanel> presetsPanel;
    std::unique_ptr<ModulationPanel> modulationPanel;
    std::unique_ptr<juce::TabbedComponent> modulationTabs;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    float displayedAudioPeak = 0.0f;
    float displayedMidiActivity = 0.0f;
    juce::ComboBox waveform;
    juce::Label waveformLabel;
    juce::Slider attack;
    juce::Slider decay;
    juce::Slider sustain;
    juce::Slider release;
    juce::Slider master;
    juce::Slider voices;
    juce::ComboBox voiceMode;
    juce::ToggleButton glideEnabled{"GLIDE"};
    juce::Slider glideTime;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label masterLabel;
    juce::Label voicesLabel;
    juce::Label voiceModeLabel;
    juce::Label glideTimeLabel;
    juce::MidiKeyboardComponent keyboard;
    juce::MidiBuffer keyboardEventDrainBuffer;

    std::unique_ptr<ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> masterAttachment;
    std::unique_ptr<SliderAttachment> voicesAttachment;
    std::unique_ptr<ComboBoxAttachment> voiceModeAttachment;
    std::unique_ptr<ButtonAttachment> glideEnabledAttachment;
    std::unique_ptr<SliderAttachment> glideTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pas1AudioProcessorEditor)
};
