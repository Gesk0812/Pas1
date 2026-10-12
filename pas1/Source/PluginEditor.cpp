#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {
const juce::Colour background{0xff101114};
const juce::Colour panel{0xff1a1c21};
const juce::Colour accent{0xffcf7bff};
const juce::Colour text{0xfff4f0f7};
const juce::Colour muted{0xffaaa4b1};

void styleSlider(juce::Slider& slider) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour{0xff393641});
    slider.setColour(juce::Slider::thumbColourId, text);
    slider.setColour(juce::Slider::textBoxTextColourId, text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void styleLabel(juce::Label& label, const juce::String& title) {
    label.setText(title, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, muted);
}
} // namespace

Pas1AudioProcessorEditor::Pas1AudioProcessorEditor(Pas1AudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      processor_(processor),
      keyboard(processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setSize(1020, 740);
    setResizable(true, true);
    setResizeLimits(980, 610, 1440, 1100);
    setLookAndFeel(nullptr);
    startTimerHz(30);
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);

    presetButton.setTooltip("Open built-in and user presets.");
    homeButton.setTooltip("Return to the performance page.");
    oscillatorButton.setTooltip("Edit oscillators, unison, FM, and noise.");
    filterButton.setTooltip("Edit the filter and amplitude/filter envelopes.");
    effectsButton.setTooltip("Edit distortion, modulation effects, delay, reverb, and EQ.");
    settingsButton.setTooltip("Set voice behavior, output, or reset parameters.");
    resetDefaultsButton.setTooltip("Reset every automatable parameter to its default value.");
    savePresetButton.setTooltip("Save the current sound as a user preset.");
    generatorButton.setTooltip("Create a sound from a text recipe or generate variations.");
    morphButton.setTooltip("Blend four captured sounds with host-automatable X/Y controls.");
    soundDNAButton.setTooltip("Analyze, compare, blend, and generate parameter-based sound profiles.");
    modulationButton.setTooltip("Route LFOs, envelopes, and MIDI controls to synth parameters.");
    engineButton.setTooltip("Open the complete automatable parameter editor.");
    waveform.setTooltip("Choose the main oscillator waveform.");
    attack.setTooltip("Time for the amplitude envelope to reach its peak.");
    decay.setTooltip("Time for the amplitude envelope to fall to sustain level.");
    sustain.setTooltip("Amplitude level held while a MIDI note is down.");
    release.setTooltip("Time for the amplitude envelope to fade after note-off.");
    master.setTooltip("Overall instrument output level.");
    voices.setTooltip("Maximum number of simultaneous notes.");
    voiceMode.setTooltip("Choose polyphonic, monophonic, or legato voice allocation.");
    glideEnabled.setTooltip("Slide between successive note pitches.");
    glideTime.setTooltip("Portamento duration for each pitch transition.");

    engineEditor = std::make_unique<juce::GenericAudioProcessorEditor>(processor_);
    addAndMakeVisible(*engineEditor);
    engineEditor->setVisible(false);
    const auto makePage = [this](const juce::String& title,
                                 std::vector<ParameterPageGroup> groups) {
        return std::make_unique<ParameterPage>(processor_.parameters, title, std::move(groups));
    };
    oscillatorPage = makePage("OSCILLATORS", {
        {"OSCILLATOR 1", {"waveform", "osc1level", "osc1octave", "osc1fine",
                          "osc1phase", "wavetablescan", "unisonvoices", "unisondetune"}},
        {"OSCILLATOR 2", {"osc2wave", "osc2level", "osc2detune", "osc2phase",
                          "osc2sync", "fmamount", "ringamount"}},
        {"OSCILLATOR 3, SUB & NOISE", {"osc3wave", "osc3level", "pulsewidth",
                                       "sublevel", "noiselevel"}}
    });
    addAndMakeVisible(*oscillatorPage);
    oscillatorPage->setVisible(false);
    filterPage = makePage("FILTERS & ENVELOPES", {
        {"FILTER", {"filtertype", "filtercutoff", "filterresonance", "filterdrive"}},
        {"FILTER MODULATION", {"filterenvamount", "filterkeytracking", "filtervelocity",
                               "filtermodwheel", "filterrandom"}},
        {"FILTER ENVELOPE", {"filterattack", "filterdecay", "filtersustain", "filterrelease"}},
        {"AMPLITUDE ENVELOPE", {"attack", "decay", "sustain", "release"}}
    });
    addAndMakeVisible(*filterPage);
    filterPage->setVisible(false);
    std::vector<ParameterPageGroup> lfoGroups;
    for (int index = 1; index <= 4; ++index) {
        const auto prefix = "lfo" + juce::String(index);
        lfoGroups.push_back({
            "LFO " + juce::String(index),
            {prefix + "wave", prefix + "rate", prefix + "amount", prefix + "target",
             prefix + "sync", prefix + "division", prefix + "phase", prefix + "fade",
             prefix + "retrigger"}
        });
    }
    lfoPage = makePage("LFO MODULATION", std::move(lfoGroups));
    effectsPage = makePage("EFFECTS", {
        {"DISTORTION", {"distortionenabled", "distortionmix", "distortiondrive"}},
        {"CHORUS", {"chorusenabled", "chorusmix", "chorusrate", "chorusdepth"}},
        {"PHASER", {"phaserenabled", "phasermix", "phaserrate", "phaserdepth"}},
        {"DELAY", {"delayenabled", "delaymix", "delaytime", "delayfeedback"}},
        {"REVERB", {"reverbenabled", "reverbmix", "reverbdecay", "reverbdamping"}},
        {"EQ & OUTPUT", {"eqenabled", "eqmix", "eqlowgain", "eqhighgain",
                         "limiterenabled", "limiterthreshold", "stereowidth"}}
    });
    addAndMakeVisible(*effectsPage);
    effectsPage->setVisible(false);
    settingsPage = makePage("SETTINGS & PERFORMANCE", {
        {"VOICE", {"polyphony", "voicemode", "glideenabled", "glidetime"}},
        {"OUTPUT", {"master", "limiterenabled", "limiterthreshold"}},
        {"HOST AUTOMATION", {"morphx", "morphy"}}
    });
    addAndMakeVisible(*settingsPage);
    settingsPage->setVisible(false);
    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(processor_);
    addAndMakeVisible(*spectrumAnalyzer);
    spectrumAnalyzer->setVisible(true);
    generatorPanel = std::make_unique<GeneratorPanel>(processor_);
    addAndMakeVisible(*generatorPanel);
    generatorPanel->setVisible(false);
    morphPanel = std::make_unique<MorphPanel>(processor_);
    addAndMakeVisible(*morphPanel);
    morphPanel->setVisible(false);
    soundDNAPanel = std::make_unique<SoundDNAPanel>(processor_);
    addAndMakeVisible(*soundDNAPanel);
    soundDNAPanel->setVisible(false);
    presetsPanel = std::make_unique<PresetsPanel>(processor_);
    addAndMakeVisible(*presetsPanel);
    presetsPanel->setVisible(false);
    modulationPanel = std::make_unique<ModulationPanel>(processor_.parameters);
    modulationTabs = std::make_unique<juce::TabbedComponent>(
        juce::TabbedButtonBar::TabsAtTop);
    modulationTabs->addTab("LFOs", accent, lfoPage.get(), false);
    modulationTabs->addTab("ROUTING MATRIX", accent, modulationPanel.get(), false);
    addAndMakeVisible(*modulationTabs);
    modulationTabs->setVisible(false);

    for (auto* button : {&homeButton, &oscillatorButton, &filterButton, &modulationButton,
                         &morphButton, &generatorButton, &soundDNAButton, &effectsButton,
                         &presetButton, &settingsButton, &resetDefaultsButton,
                         &storeSnapshotAButton, &storeSnapshotBButton,
                         &compareSnapshotsButton, &savePresetButton, &engineButton}) {
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::textColourOffId, text);
        button->setColour(juce::TextButton::textColourOnId, accent);
        button->setClickingTogglesState(button != &savePresetButton
                                        && button != &resetDefaultsButton
                                        && button != &storeSnapshotAButton
                                        && button != &storeSnapshotBButton);
        addAndMakeVisible(*button);
    }
    resetDefaultsButton.setVisible(false);
    storeSnapshotAButton.setVisible(false);
    storeSnapshotBButton.setVisible(false);
    compareSnapshotsButton.setVisible(false);

    homeButton.onClick = [this] { showPage(Page::home); };
    oscillatorButton.onClick = [this] { showPage(Page::oscillators); };
    filterButton.onClick = [this] { showPage(Page::filters); };
    modulationButton.onClick = [this] { showPage(Page::modulation); };
    morphButton.onClick = [this] { showPage(Page::morph); };
    generatorButton.onClick = [this] { showPage(Page::generator); };
    soundDNAButton.onClick = [this] { showPage(Page::soundDNA); };
    effectsButton.onClick = [this] { showPage(Page::effects); };
    presetButton.onClick = [this] { showPage(Page::presets); };
    settingsButton.onClick = [this] { showPage(Page::settings); };
    engineButton.onClick = [this] {
        showPage(currentPage == Page::allParameters ? Page::home : Page::allParameters);
    };
    savePresetButton.onClick = [this] { savePreset(); };
    storeSnapshotAButton.onClick = [this] {
        snapshotA.setSize(0);
        processor_.getStateInformation(snapshotA);
        showingSnapshotB = false;
        compareSnapshotsButton.setButtonText("SHOW B");
        compareSnapshotsButton.setToggleState(false, juce::dontSendNotification);
    };
    storeSnapshotBButton.onClick = [this] {
        snapshotB.setSize(0);
        processor_.getStateInformation(snapshotB);
        showingSnapshotB = true;
        compareSnapshotsButton.setButtonText("SHOW A");
        compareSnapshotsButton.setToggleState(true, juce::dontSendNotification);
    };
    compareSnapshotsButton.setButtonText("SHOW B");
    compareSnapshotsButton.onClick = [this] {
        auto& target = showingSnapshotB ? snapshotA : snapshotB;
        if (target.getSize() == 0) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon, "PAS 1 A/B",
                "Store snapshots A and B before switching between them.", "OK", this);
            compareSnapshotsButton.setToggleState(showingSnapshotB,
                                                   juce::dontSendNotification);
            return;
        }
        processor_.setStateInformation(target.getData(),
                                       static_cast<int>(target.getSize()));
        showingSnapshotB = !showingSnapshotB;
        compareSnapshotsButton.setButtonText(showingSnapshotB ? "SHOW A" : "SHOW B");
    };
    resetDefaultsButton.onClick = [this] {
        juce::Component::SafePointer<Pas1AudioProcessorEditor> safeThis(this);
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon, "Reset PAS 1 parameters?",
            "All automatable parameters will return to their factory values. "
            "This changes the current sound.",
            "Reset", "Cancel", this,
            juce::ModalCallbackFunction::create([safeThis](int result) {
                if (safeThis == nullptr || result != 1)
                    return;
                for (auto* parameter : safeThis->processor_.getParameters()) {
                    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter)) {
                        ranged->beginChangeGesture();
                        ranged->setValueNotifyingHost(ranged->getDefaultValue());
                        ranged->endChangeGesture();
                    }
                }
            }));
    };

    waveform.addItemList({"Sine", "Triangle", "Saw", "Square"}, 1);
    waveform.setColour(juce::ComboBox::backgroundColourId, panel);
    waveform.setColour(juce::ComboBox::textColourId, text);
    waveform.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff393641});
    waveform.setColour(juce::ComboBox::arrowColourId, accent);
    addAndMakeVisible(waveform);
    styleLabel(waveformLabel, "OSCILLATOR");
    addAndMakeVisible(waveformLabel);

    for (auto* slider : {&attack, &decay, &sustain, &release, &master, &voices}) {
        styleSlider(*slider);
        addAndMakeVisible(*slider);
    }
    styleLabel(attackLabel, "ATTACK");
    styleLabel(decayLabel, "DECAY");
    styleLabel(sustainLabel, "SUSTAIN");
    styleLabel(releaseLabel, "RELEASE");
    styleLabel(masterLabel, "OUTPUT");
    styleLabel(voicesLabel, "VOICES");
    for (auto* label : {&attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &masterLabel, &voicesLabel})
        addAndMakeVisible(*label);

    voices.setNumDecimalPlacesToDisplay(0);
    voices.setTextValueSuffix(" voices");
    attack.setTextValueSuffix(" s");
    decay.setTextValueSuffix(" s");
    release.setTextValueSuffix(" s");

    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour{0xffeeeaf0});
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour{0xff34303b});
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, background);
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accent.withAlpha(0.45f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent.withAlpha(0.72f));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, muted);
    addAndMakeVisible(keyboard);

    auto& state = processor_.parameters;
    waveformAttachment = std::make_unique<ComboBoxAttachment>(state, "waveform", waveform);
    attackAttachment = std::make_unique<SliderAttachment>(state, "attack", attack);
    decayAttachment = std::make_unique<SliderAttachment>(state, "decay", decay);
    sustainAttachment = std::make_unique<SliderAttachment>(state, "sustain", sustain);
    releaseAttachment = std::make_unique<SliderAttachment>(state, "release", release);
    masterAttachment = std::make_unique<SliderAttachment>(state, "master", master);
    voicesAttachment = std::make_unique<SliderAttachment>(state, "polyphony", voices);

    voiceMode.addItemList({"Poly", "Mono", "Legato"}, 1);
    voiceMode.setColour(juce::ComboBox::backgroundColourId, panel);
    voiceMode.setColour(juce::ComboBox::textColourId, text);
    voiceMode.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff393641});
    voiceMode.setColour(juce::ComboBox::arrowColourId, accent);
    addAndMakeVisible(voiceMode);
    styleLabel(voiceModeLabel, "VOICE MODE");
    addAndMakeVisible(voiceModeLabel);

    glideEnabled.setClickingTogglesState(true);
    glideEnabled.setColour(juce::ToggleButton::textColourId, text);
    glideEnabled.setColour(juce::ToggleButton::tickColourId, accent);
    glideEnabled.setColour(juce::ToggleButton::tickDisabledColourId, muted);
    addAndMakeVisible(glideEnabled);
    glideTime.setSliderStyle(juce::Slider::LinearHorizontal);
    glideTime.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 22);
    glideTime.setRange(0.001, 2.0, 0.001);
    glideTime.setTextValueSuffix(" s");
    glideTime.setColour(juce::Slider::trackColourId, accent);
    glideTime.setColour(juce::Slider::thumbColourId, text);
    glideTime.setColour(juce::Slider::textBoxTextColourId, text);
    glideTime.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    glideTime.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour{0xff393641});
    addAndMakeVisible(glideTime);
    styleLabel(glideTimeLabel, "GLIDE TIME");
    addAndMakeVisible(glideTimeLabel);
    voiceModeAttachment = std::make_unique<ComboBoxAttachment>(state, "voicemode", voiceMode);
    glideEnabledAttachment = std::make_unique<ButtonAttachment>(
        state, "glideenabled", glideEnabled);
    glideTimeAttachment = std::make_unique<SliderAttachment>(state, "glidetime", glideTime);
    showPage(Page::home);
}

void Pas1AudioProcessorEditor::paint(juce::Graphics& graphics) {
    graphics.fillAll(background);

    graphics.setColour(juce::Colour{0xff24212a});
    graphics.fillRoundedRectangle(getLocalBounds().toFloat().reduced(14.0f), 16.0f);

    graphics.setColour(text);
    graphics.setFont(juce::FontOptions{32.0f, juce::Font::bold});
    graphics.drawText("PAS 1", 36, 18, 220, 40, juce::Justification::centredLeft);
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions{11.0f});
    const auto subtitle = [this]() -> juce::String {
        switch (currentPage) {
            case Page::home: return "ADAPTIVE SYNTHESIZER  /  PERFORMANCE";
            case Page::oscillators: return "SOUND DESIGN  /  OSCILLATORS";
            case Page::filters: return "SOUND DESIGN  /  FILTERS & ENVELOPES";
            case Page::modulation: return "SOUND DESIGN  /  MODULATION";
            case Page::morph: return "MORPH MATRIX  /  AUTOMATABLE X-Y";
            case Page::generator: return "SOUND RECIPE  /  VARIATIONS";
            case Page::soundDNA: return "SOUND DNA  /  PROFILE ANALYSIS";
            case Page::effects: return "SOUND DESIGN  /  EFFECTS";
            case Page::presets: return "PRESET LIBRARY  /  SEARCH & ORGANIZE";
            case Page::settings: return "PLUGIN  /  SETTINGS & PERFORMANCE";
            case Page::allParameters: return "ENGINE  /  COMPLETE PARAMETER LIST";
        }
        return "ADAPTIVE SYNTHESIZER";
    }();
    graphics.drawText(subtitle, 38, 56, 380, 18, juce::Justification::centredLeft);

    graphics.setFont(juce::FontOptions{9.0f, juce::Font::bold});
    graphics.setColour(muted);
    graphics.drawText("MIDI", 420, 40, 34, 12, juce::Justification::centredLeft);
    graphics.setColour(displayedMidiActivity > 0.06f ? accent : juce::Colour{0xff393641});
    graphics.fillEllipse(451.0f, 41.0f, 9.0f, 9.0f);
    graphics.setColour(muted);
    graphics.drawText("AUDIO", 470, 40, 38, 12, juce::Justification::centredLeft);
    const juce::Rectangle<float> meterTrack{510.0f, 42.0f,
                                            static_cast<float>(std::max(80, getWidth() - 770)), 7.0f};
    graphics.setColour(juce::Colour{0xff393641});
    graphics.fillRoundedRectangle(meterTrack, 3.5f);
    auto meterFill = meterTrack;
    meterFill.setWidth(meterFill.getWidth() * std::clamp(displayedAudioPeak, 0.0f, 1.0f));
    graphics.setColour(displayedAudioPeak > 0.92f ? juce::Colour{0xffff665e} : accent);
    graphics.fillRoundedRectangle(meterFill, 3.5f);

    if (currentPage != Page::home) {
        graphics.setColour(panel);
        graphics.fillRoundedRectangle(30.0f, 116.0f, static_cast<float>(getWidth() - 60),
                                      static_cast<float>(getHeight() - 144), 12.0f);
        return;
    }

    graphics.setColour(accent.withAlpha(0.14f));
    graphics.fillEllipse(static_cast<float>(getWidth() - 290), -105.0f, 300.0f, 300.0f);

    graphics.setColour(panel);
    graphics.fillRoundedRectangle(30.0f, 120.0f, static_cast<float>(getWidth() - 60), 82.0f, 12.0f);
    graphics.fillRoundedRectangle(30.0f, 212.0f, static_cast<float>(getWidth() - 60), 176.0f, 12.0f);
    graphics.fillRoundedRectangle(30.0f, 398.0f, static_cast<float>(getWidth() - 60), 158.0f, 12.0f);

    graphics.setColour(accent);
    graphics.setFont(juce::FontOptions{10.0f, juce::Font::bold});
    graphics.drawText("SOUND SOURCE", 48, 130, 200, 16, juce::Justification::centredLeft);
    graphics.drawText("AMPLITUDE ENVELOPE", 48, 222, 250, 16, juce::Justification::centredLeft);
    graphics.drawText("PLAY", 48, 408, 100, 16, juce::Justification::centredLeft);

    const auto readValue = [this](const char* id, float fallback) {
        if (const auto* value = processor_.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);
        return fallback;
    };
    const auto waveLeft = 400.0f;
    const auto waveRight = static_cast<float>(std::min(740, getWidth() - 250));
    const auto waveTop = 145.0f;
    const auto waveBottom = 180.0f;
    graphics.setColour(juce::Colour{0xff393641});
    graphics.drawLine(waveLeft, (waveTop + waveBottom) * 0.5f,
                      waveRight, (waveTop + waveBottom) * 0.5f, 1.0f);
    juce::Path wavePath;
    const auto waveType = juce::roundToInt(readValue("waveform", 0.0f));
    for (int sample = 0; sample <= 64; ++sample) {
        const auto phase = static_cast<float>(sample) / 64.0f;
        const auto radians = phase * juce::MathConstants<float>::twoPi;
        float value = 0.0f;
        switch (waveType) {
            case 1: value = 1.0f - 4.0f * std::abs(phase - 0.5f); break;
            case 2: value = phase * 2.0f - 1.0f; break;
            case 3: value = phase < 0.5f ? 1.0f : -1.0f; break;
            default: value = std::sin(radians); break;
        }
        const auto x = waveLeft + (waveRight - waveLeft) * phase;
        const auto y = (waveTop + waveBottom) * 0.5f - value * 15.0f;
        if (sample == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }
    graphics.setColour(accent);
    graphics.strokePath(wavePath, juce::PathStrokeType{1.7f});

    const auto left = static_cast<float>(std::max(550, getWidth() / 2 + 50));
    const auto right = static_cast<float>(getWidth() - 54);
    const auto top = 252.0f;
    const auto bottom = 358.0f;
    const auto graphWidth = right - left;
    graphics.setColour(juce::Colour{0xff393641});
    for (int line = 1; line <= 3; ++line) {
        const auto y = top + (bottom - top) * static_cast<float>(line) / 4.0f;
        graphics.drawLine(left, y, right, y, 1.0f);
    }
    const auto attackSeconds = std::max(0.001f, readValue("attack", 0.01f));
    const auto decaySeconds = std::max(0.001f, readValue("decay", 0.15f));
    const auto releaseSeconds = std::max(0.001f, readValue("release", 0.2f));
    const auto sustainLevel = std::clamp(readValue("sustain", 0.75f), 0.0f, 1.0f);
    const auto total = attackSeconds + decaySeconds + releaseSeconds;
    const auto attackX = left + graphWidth * attackSeconds / total;
    const auto decayX = attackX + graphWidth * decaySeconds / total;
    const auto sustainY = bottom - sustainLevel * (bottom - top);
    juce::Path envelope;
    envelope.startNewSubPath(left, bottom);
    envelope.lineTo(attackX, top);
    envelope.lineTo(decayX, sustainY);
    envelope.lineTo(right - graphWidth * releaseSeconds / total, sustainY);
    envelope.lineTo(right, bottom);
    graphics.setColour(accent);
    graphics.strokePath(envelope, juce::PathStrokeType{2.0f});
}

void Pas1AudioProcessorEditor::timerCallback() {
    // Drain JUCE's UI-side keyboard event staging buffer off the audio thread.
    processor_.keyboardState.processNextMidiBuffer(keyboardEventDrainBuffer, 0, 1, false);
    keyboardEventDrainBuffer.clear();
    const auto audioTarget = processor_.outputPeak.load(std::memory_order_relaxed);
    const auto midiTarget = processor_.midiActivity.load(std::memory_order_relaxed);
    displayedAudioPeak = std::max(audioTarget, displayedAudioPeak * 0.82f);
    displayedMidiActivity = std::max(midiTarget, displayedMidiActivity * 0.82f);
    repaint(416, 34, getWidth() - 446, 24);
    if (currentPage == Page::home)
        spectrumAnalyzer->update();
}

void Pas1AudioProcessorEditor::resized() {
    const auto width = getWidth();
    const std::array<std::pair<juce::TextButton*, int>, 10> tabs{{
        {&homeButton, 58}, {&oscillatorButton, 62}, {&filterButton, 68},
        {&modulationButton, 70}, {&morphButton, 62}, {&generatorButton, 72},
        {&soundDNAButton, 68}, {&effectsButton, 48}, {&presetButton, 70},
        {&settingsButton, 76}
    }};
    auto tabX = 30;
    for (const auto& [button, tabWidth] : tabs) {
        button->setBounds(tabX, 76, tabWidth, 29);
        tabX += tabWidth + 4;
    }
    savePresetButton.setBounds(width - 250, 20, 78, 30);
    engineButton.setBounds(width - 164, 20, 128, 30);

    const auto pageBounds = juce::Rectangle<int>{30, 116, width - 60, getHeight() - 142};
    for (auto* page : {static_cast<juce::Component*>(oscillatorPage.get()),
                       static_cast<juce::Component*>(filterPage.get()),
                       static_cast<juce::Component*>(effectsPage.get()),
                       static_cast<juce::Component*>(settingsPage.get()),
                       static_cast<juce::Component*>(generatorPanel.get()),
                       static_cast<juce::Component*>(morphPanel.get()),
                       static_cast<juce::Component*>(soundDNAPanel.get()),
                       static_cast<juce::Component*>(presetsPanel.get()),
                       static_cast<juce::Component*>(modulationTabs.get()),
                       static_cast<juce::Component*>(engineEditor.get())})
        page->setBounds(pageBounds);
    resetDefaultsButton.setBounds(46, 121, 174, 27);
    storeSnapshotAButton.setBounds(230, 121, 72, 27);
    storeSnapshotBButton.setBounds(308, 121, 72, 27);
    compareSnapshotsButton.setBounds(386, 121, 92, 27);
    settingsPage->setBounds(pageBounds.withY(152).withHeight(pageBounds.getHeight() - 36));

    waveformLabel.setBounds(50, 145, 125, 18);
    waveform.setBounds(180, 140, 190, 34);

    const auto sliderWidth = 104;
    const auto gap = 18;
    auto x = 48;
    const auto sliderY = 253;
    for (auto* item : {&attack, &decay, &sustain, &release}) {
        item->setBounds(x, sliderY, sliderWidth, 112);
        x += sliderWidth + gap;
    }
    attackLabel.setBounds(attack.getX(), 234, sliderWidth, 18);
    decayLabel.setBounds(decay.getX(), 234, sliderWidth, 18);
    sustainLabel.setBounds(sustain.getX(), 234, sliderWidth, 18);
    releaseLabel.setBounds(release.getX(), 234, sliderWidth, 18);

    const auto rightControlsX = std::max(520, width - 230);
    master.setBounds(rightControlsX, 137, 92, 55);
    voices.setBounds(rightControlsX + 104, 137, 92, 55);
    masterLabel.setBounds(master.getX(), 122, 92, 18);
    voicesLabel.setBounds(voices.getX(), 122, 92, 18);
    voiceModeLabel.setBounds(48, 422, 144, 14);
    voiceMode.setBounds(48, 438, 144, 29);
    glideEnabled.setBounds(204, 438, 86, 29);
    glideTimeLabel.setBounds(304, 422, 160, 14);
    glideTime.setBounds(304, 436, 210, 34);
    keyboard.setBounds(48, 482, width - 96, 54);
    spectrumAnalyzer->setBounds(540, 420, width - 594, 52);
}

void Pas1AudioProcessorEditor::setCoreControlsVisible(bool visible) {
    for (auto* component : {static_cast<juce::Component*>(&waveform),
                            static_cast<juce::Component*>(&waveformLabel),
                            static_cast<juce::Component*>(&attack),
                            static_cast<juce::Component*>(&decay),
                            static_cast<juce::Component*>(&sustain),
                            static_cast<juce::Component*>(&release),
                            static_cast<juce::Component*>(&master),
                            static_cast<juce::Component*>(&voices),
                            static_cast<juce::Component*>(&voiceMode),
                            static_cast<juce::Component*>(&glideEnabled),
                            static_cast<juce::Component*>(&glideTime),
                            static_cast<juce::Component*>(&voiceModeLabel),
                            static_cast<juce::Component*>(&glideTimeLabel),
                            static_cast<juce::Component*>(&attackLabel),
                            static_cast<juce::Component*>(&decayLabel),
                            static_cast<juce::Component*>(&sustainLabel),
                            static_cast<juce::Component*>(&releaseLabel),
                            static_cast<juce::Component*>(&masterLabel),
                            static_cast<juce::Component*>(&voicesLabel),
                            static_cast<juce::Component*>(&keyboard)})
        component->setVisible(visible);
}

void Pas1AudioProcessorEditor::showPage(Page page) {
    currentPage = page;
    const auto home = page == Page::home;
    engineEditor->setVisible(page == Page::allParameters);
    oscillatorPage->setVisible(page == Page::oscillators);
    filterPage->setVisible(page == Page::filters);
    modulationTabs->setVisible(page == Page::modulation);
    morphPanel->setVisible(page == Page::morph);
    generatorPanel->setVisible(page == Page::generator);
    soundDNAPanel->setVisible(page == Page::soundDNA);
    effectsPage->setVisible(page == Page::effects);
    presetsPanel->setVisible(page == Page::presets);
    settingsPage->setVisible(page == Page::settings);
    spectrumAnalyzer->setVisible(home);
    resetDefaultsButton.setVisible(page == Page::settings);
    storeSnapshotAButton.setVisible(page == Page::settings);
    storeSnapshotBButton.setVisible(page == Page::settings);
    compareSnapshotsButton.setVisible(page == Page::settings);
    setCoreControlsVisible(home);

    homeButton.setToggleState(page == Page::home, juce::dontSendNotification);
    oscillatorButton.setToggleState(page == Page::oscillators, juce::dontSendNotification);
    filterButton.setToggleState(page == Page::filters, juce::dontSendNotification);
    modulationButton.setToggleState(page == Page::modulation, juce::dontSendNotification);
    morphButton.setToggleState(page == Page::morph, juce::dontSendNotification);
    generatorButton.setToggleState(page == Page::generator, juce::dontSendNotification);
    soundDNAButton.setToggleState(page == Page::soundDNA, juce::dontSendNotification);
    effectsButton.setToggleState(page == Page::effects, juce::dontSendNotification);
    presetButton.setToggleState(page == Page::presets, juce::dontSendNotification);
    settingsButton.setToggleState(page == Page::settings, juce::dontSendNotification);
    engineButton.setToggleState(page == Page::allParameters, juce::dontSendNotification);

    if (page == Page::presets)
        presetsPanel->refreshLibrary();

    resized();
    repaint();
}

void Pas1AudioProcessorEditor::savePreset() {
    auto* alert = new juce::AlertWindow("Save PAS 1 Preset", "Save the current sound to your preset library.",
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("presetName", "My Preset", "Name");
    alert->addTextEditor("presetCategory", "User Presets", "Category");
    alert->addButton("Save", 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton("Cancel", 0, juce::KeyPress{juce::KeyPress::escapeKey});
    juce::Component::SafePointer<juce::AlertWindow> safeAlert(alert);
    juce::Component::SafePointer<Pas1AudioProcessorEditor> safeThis(this);
    alert->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, safeAlert](int result) {
        if (safeThis == nullptr || safeAlert == nullptr || result != 1)
            return;
        juce::File savedFile;
        juce::String error;
        if (!safeThis->processor_.presetManager.saveUserPreset(
                safeAlert->getTextEditorContents("presetName"),
                safeAlert->getTextEditorContents("presetCategory"), savedFile, error))
            safeThis->showPresetError(error);
        else
            safeThis->presetsPanel->refreshLibrary();
    }), true);
}

void Pas1AudioProcessorEditor::showPresetError(const juce::String& message) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "PAS 1 Presets", message, "OK", this);
}
