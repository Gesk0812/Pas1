#include "PluginEditor.h"
#include "Diagnostics/StartupTrace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace {
const juce::Colour background{0xff180e09};
const juce::Colour panel{0xff302015};
const juce::Colour accent{0xffd8b36a};
const juce::Colour text{0xfff4e8cf};
const juce::Colour muted{0xffbba483};
const juce::Colour brassTrim{0xff745332};
const juce::Colour darkWood{0xff21130d};

void styleSlider(juce::Slider& slider) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, brassTrim);
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

struct EnvelopePlot {
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
    float attackX = 0.0f;
    float decayX = 0.0f;
    float sustainY = 0.0f;
    float releaseX = 0.0f;
    float holdFraction = 0.22f;
};

EnvelopePlot makeEnvelopePlot(int editorWidth, float attack, float decay,
                              float sustain, float release) {
    EnvelopePlot plot;
    plot.left = static_cast<float>(std::max(550, editorWidth / 2 + 50));
    plot.right = static_cast<float>(editorWidth - 54);
    plot.top = 252.0f;
    plot.bottom = 358.0f;
    const auto width = plot.right - plot.left;
    const auto total = std::max(0.001f, attack + decay + release);
    const auto timedWidth = width * (1.0f - plot.holdFraction);
    plot.attackX = plot.left + timedWidth * attack / total;
    plot.decayX = plot.attackX + timedWidth * decay / total;
    plot.sustainY = plot.bottom - std::clamp(sustain, 0.0f, 1.0f)
                                      * (plot.bottom - plot.top);
    plot.releaseX = plot.decayX + width * plot.holdFraction;
    return plot;
}
} // namespace

Pas1AudioProcessorEditor::Pas1AudioProcessorEditor(Pas1AudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      processor_(processor),
      keyboard(processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard) {
    pas1::diagnostics::startupTrace("editor.constructor.begin");
    setResizable(true, true);
    setResizeLimits(980, 610, 1440, 1100);
    setLookAndFeel(nullptr);
    startTimerHz(30);
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);

    presetButton.setTooltip(pas1::ui::utf8String("Открывает заводские и пользовательские пресеты."));
    homeButton.setTooltip(pas1::ui::utf8String("Возвращает на главную страницу управления звуком."));
    oscillatorButton.setTooltip(pas1::ui::utf8String("Настройка осцилляторов, унисона, ЧМ и шума."));
    filterButton.setTooltip(pas1::ui::utf8String("Настройка фильтра и амплитудной/фильтровой огибающих."));
    effectsButton.setTooltip(pas1::ui::utf8String("Настройка искажений, модуляции, задержки, реверберации и эквалайзера."));
    settingsButton.setTooltip(pas1::ui::utf8String("Настройка голосов, выходного сигнала и сброс параметров."));
    resetDefaultsButton.setTooltip(pas1::ui::utf8String("Сбрасывает параметры к заводским значениям и очищает настройки/снимки A/B в матрице морфинга. Пользовательские пресеты остаются."));
    savePresetButton.setTooltip(pas1::ui::utf8String("Сохраняет текущий звук как новый пользовательский пресет."));
    generatorButton.setTooltip(pas1::ui::utf8String("Создаёт звук по короткому текстовому рецепту или делает вариации."));
    morphButton.setTooltip(pas1::ui::utf8String("Смешивает четыре сохранённых звука при перемещении точки X/Y."));
    soundDNAButton.setTooltip(pas1::ui::utf8String("Анализирует и сравнивает тембры по параметрам синтезатора."));
    modulationButton.setTooltip(pas1::ui::utf8String("Направляет низкочастотные генераторы, огибающие и MIDI-контроллеры на параметры синтезатора."));
    engineButton.setTooltip(pas1::ui::utf8String("Показывает полный список автоматизируемых параметров."));
    waveform.setTooltip(pas1::ui::utf8String("Выберите форму волны основного осциллятора."));
    attack.setTooltip(pas1::ui::utf8String("Время нарастания громкости после нажатия клавиши."));
    decay.setTooltip(pas1::ui::utf8String("Время перехода от пика атаки к уровню удержания."));
    sustain.setTooltip(pas1::ui::utf8String("Уровень громкости, пока удерживается MIDI-нота."));
    release.setTooltip(pas1::ui::utf8String("Время затухания после отпускания MIDI-ноты."));
    master.setTooltip(pas1::ui::utf8String("Общая громкость инструмента на выходе."));
    voices.setTooltip(pas1::ui::utf8String("Максимальное количество одновременно звучащих нот."));
    voiceMode.setTooltip(pas1::ui::utf8String("Полифония позволяет играть несколько нот; монофония — только одну; легато связывает ноты, сыгранные без отпускания предыдущей."));
    glideEnabled.setTooltip(pas1::ui::utf8String("Включает скольжение высоты между нотами."));
    glideTime.setTooltip(pas1::ui::utf8String("Длительность перехода между высотами нот при включённом глиссандо."));

    pas1::diagnostics::startupTrace("editor.genericEditor.before");
    engineEditor = std::make_unique<juce::GenericAudioProcessorEditor>(processor_);
    pas1::diagnostics::startupTrace("editor.genericEditor.created");
    addAndMakeVisible(*engineEditor);
    engineEditor->setVisible(false);
    const auto makePage = [this](const juce::String& title,
                                 std::vector<ParameterPageGroup> groups) {
        return std::make_unique<ParameterPage>(processor_.parameters, title, std::move(groups));
    };
    pas1::diagnostics::startupTrace("editor.oscillatorPage.before");
    oscillatorWaveformGraph = std::make_unique<OscillatorWaveformGraph>(processor_);
    oscillatorPage = std::make_unique<ParameterPage>(
        processor_.parameters, pas1::ui::utf8String("ОСЦИЛЛЯТОРЫ"),
        std::vector<ParameterPageGroup>{
            {pas1::ui::utf8String("ОСЦИЛЛЯТОР 1"), {"waveform", "osc1level", "osc1octave", "osc1fine",
                              "osc1phase", "wavetablescan", "unisonvoices", "unisondetune"}},
            {pas1::ui::utf8String("ОСЦИЛЛЯТОР 2"), {"osc2wave", "osc2level", "osc2detune", "osc2phase",
                              "osc2sync", "fmamount", "ringamount"}},
            {pas1::ui::utf8String("ОСЦИЛЛЯТОР 3, САБ И ШУМ"), {"osc3wave", "osc3level", "pulsewidth",
                                           "sublevel", "noiselevel"}},
            {pas1::ui::utf8String("МЕТАЛЛИЧЕСКИЕ ЯЗЫЧКИ"), {"musicboxmix", "musicboxbrightness", "musicboxdecay",
                                      "musicboxresonance", "musicboxinharmonicity", "musicboxstrike"}}
        },
        ParameterPageTheme{}, oscillatorWaveformGraph.get(), 116);
    pas1::diagnostics::startupTrace("editor.oscillatorPage.created");
    addAndMakeVisible(*oscillatorPage);
    oscillatorPage->setVisible(false);
    const ParameterPageTheme ancientTheme{
        juce::Colour{0xff291523}, juce::Colour{0xff42243a},
        juce::Colour{0xffef8fc3}, juce::Colour{0xffffedf6},
        juce::Colour{0xffd9a7c0}, juce::Colour{0xff8d5675}
    };
    ancientPage = std::make_unique<ParameterPage>(
        processor_.parameters, pas1::ui::utf8String("ДРЕВНИЕ ТЕМБРЫ / АУЛОС · ЛИРА · БРОНЗА"),
        std::vector<ParameterPageGroup>{
            {pas1::ui::utf8String("ИНСТРУМЕНТ И СМЕСЬ"), {"ancientinstrument", "ancientmix", "ancientbrightness"}},
            {pas1::ui::utf8String("РЕЗОНАНС И ТЕМБР"), {"ancientdecay", "ancientair", "ancientdetune"}}
        },
        ancientTheme);
    addAndMakeVisible(*ancientPage);
    ancientPage->setVisible(false);
    pas1::diagnostics::startupTrace("editor.filterPage.before");
    filterGraphs = std::make_unique<FilterGraphs>(processor_);
    filterGraphs->setTooltip(pas1::ui::utf8String(
        "Частотный график: перетащите точку — по горизонтали меняется частота среза, по вертикали резонанс. На нижнем графике тяните точки огибающей: атака, спад/уровень удержания, отпускание."));
    filterPage = std::make_unique<ParameterPage>(
        processor_.parameters, pas1::ui::utf8String("ФИЛЬТРЫ И ОГИБАЮЩИЕ"),
        std::vector<ParameterPageGroup>{
            {pas1::ui::utf8String("ФИЛЬТР"), {"filtertype", "filtercutoff", "filterresonance", "filterdrive"}},
            {pas1::ui::utf8String("МОДУЛЯЦИЯ ФИЛЬТРА"), {"filterenvamount", "filterkeytracking", "filtervelocity",
                                   "filtermodwheel", "filterrandom"}},
            {pas1::ui::utf8String("ОГИБАЮЩАЯ ФИЛЬТРА"), {"filterattack", "filterdecay", "filtersustain", "filterrelease"}},
            {pas1::ui::utf8String("АМПЛИТУДНАЯ ОГИБАЮЩАЯ"), {"attack", "decay", "sustain", "release"}}
        },
        ParameterPageTheme{}, filterGraphs.get(), 214);
    pas1::diagnostics::startupTrace("editor.filterPage.created");
    addAndMakeVisible(*filterPage);
    filterPage->setVisible(false);
    std::vector<ParameterPageGroup> lfoGroups;
    for (int index = 1; index <= 4; ++index) {
        const auto prefix = "lfo" + juce::String(index);
        lfoGroups.push_back({
            pas1::ui::utf8String("НЧ-ГЕНЕРАТОР ") + juce::String(index),
            {prefix + "wave", prefix + "rate", prefix + "amount", prefix + "target",
             prefix + "sync", prefix + "division", prefix + "phase", prefix + "fade",
             prefix + "retrigger"}
        });
    }
    pas1::diagnostics::startupTrace("editor.lfoPage.before");
    lfoPage = makePage(pas1::ui::utf8String("МОДУЛЯЦИЯ НЧ-ГЕНЕРАТОРОВ"), std::move(lfoGroups));
    pas1::diagnostics::startupTrace("editor.lfoPage.created");
    pas1::diagnostics::startupTrace("editor.effectsPage.before");
    effectsPage = makePage(pas1::ui::utf8String("ЭФФЕКТЫ"), {
        {pas1::ui::utf8String("ИСКАЖЕНИЕ"), {"distortionenabled", "distortionmix", "distortiondrive"}},
        {pas1::ui::utf8String("ХОРУС"), {"chorusenabled", "chorusmix", "chorusrate", "chorusdepth"}},
        {pas1::ui::utf8String("ФАЗЕР"), {"phaserenabled", "phasermix", "phaserrate", "phaserdepth"}},
        {pas1::ui::utf8String("ЗАДЕРЖКА"), {"delayenabled", "delaymix", "delaytime", "delayfeedback"}},
        {pas1::ui::utf8String("РЕВЕРБЕРАЦИЯ"), {"reverbenabled", "reverbmix", "reverbdecay", "reverbdamping"}},
        {pas1::ui::utf8String("ЭКВАЛАЙЗЕР И ВЫХОД"), {"eqenabled", "eqmix", "eqlowgain", "eqhighgain",
                         "limiterenabled", "limiterthreshold", "stereowidth"}}
    });
    pas1::diagnostics::startupTrace("editor.effectsPage.created");
    addAndMakeVisible(*effectsPage);
    effectsPage->setVisible(false);
    pas1::diagnostics::startupTrace("editor.settingsPage.before");
    settingsPage = makePage(pas1::ui::utf8String("НАСТРОЙКИ И ИСПОЛНЕНИЕ"), {
        {pas1::ui::utf8String("ГОЛОСА"), {"polyphony", "voicemode", "glideenabled", "glidetime"}},
        {pas1::ui::utf8String("ВЫХОД"), {"master", "limiterenabled", "limiterthreshold"}},
        {pas1::ui::utf8String("АВТОМАТИЗАЦИЯ В ХОСТЕ"), {"morphx", "morphy"}}
    });
    pas1::diagnostics::startupTrace("editor.settingsPage.created");
    addAndMakeVisible(*settingsPage);
    settingsPage->setVisible(false);
    pas1::diagnostics::startupTrace("editor.spectrumAnalyzer.before");
    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(processor_);
    pas1::diagnostics::startupTrace("editor.spectrumAnalyzer.created");
    addAndMakeVisible(*spectrumAnalyzer);
    spectrumAnalyzer->setVisible(true);
    pas1::diagnostics::startupTrace("editor.generatorPanel.before");
    generatorPanel = std::make_unique<GeneratorPanel>(processor_);
    pas1::diagnostics::startupTrace("editor.generatorPanel.created");
    addAndMakeVisible(*generatorPanel);
    generatorPanel->setVisible(false);
    pas1::diagnostics::startupTrace("editor.morphPanel.before");
    morphPanel = std::make_unique<MorphPanel>(processor_);
    pas1::diagnostics::startupTrace("editor.morphPanel.created");
    addAndMakeVisible(*morphPanel);
    morphPanel->setVisible(false);
    pas1::diagnostics::startupTrace("editor.soundDNAPanel.before");
    soundDNAPanel = std::make_unique<SoundDNAPanel>(processor_);
    pas1::diagnostics::startupTrace("editor.soundDNAPanel.created");
    addAndMakeVisible(*soundDNAPanel);
    soundDNAPanel->setVisible(false);
    pas1::diagnostics::startupTrace("editor.presetsPanel.before");
    presetsPanel = std::make_unique<PresetsPanel>(processor_);
    pas1::diagnostics::startupTrace("editor.presetsPanel.created");
    addAndMakeVisible(*presetsPanel);
    presetsPanel->setVisible(false);
    pas1::diagnostics::startupTrace("editor.modulationPanel.before");
    modulationPanel = std::make_unique<ModulationPanel>(processor_.parameters);
    pas1::diagnostics::startupTrace("editor.modulationPanel.created");
    modulationTabs = std::make_unique<juce::TabbedComponent>(
        juce::TabbedButtonBar::TabsAtTop);
    modulationTabs->addTab(pas1::ui::utf8String("НЧ-ГЕНЕРАТОРЫ"), accent, lfoPage.get(), false);
    modulationTabs->addTab(pas1::ui::utf8String("МАТРИЦА МАРШРУТИЗАЦИИ"), accent, modulationPanel.get(), false);
    addAndMakeVisible(*modulationTabs);
    modulationTabs->setVisible(false);

    for (auto* button : {&homeButton, &oscillatorButton, &ancientButton, &filterButton, &modulationButton,
                         &morphButton, &generatorButton, &soundDNAButton, &effectsButton,
                         &presetButton, &settingsButton, &resetDefaultsButton,
                         &storeSnapshotAButton, &storeSnapshotBButton,
                         &compareSnapshotsButton, &savePresetButton, &engineButton}) {
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour{0xff49331f});
        button->setColour(juce::TextButton::textColourOffId, text);
        button->setColour(juce::TextButton::textColourOnId, accent);
        button->setClickingTogglesState(button != &savePresetButton
                                        && button != &resetDefaultsButton
                                        && button != &storeSnapshotAButton
                                        && button != &storeSnapshotBButton);
        addAndMakeVisible(*button);
    }
    ancientButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour{0xff65314d});
    ancientButton.setColour(juce::TextButton::textColourOnId, juce::Colour{0xffffb6db});
    resetDefaultsButton.setButtonText(pas1::ui::utf8String("СБРОС"));
    storeSnapshotAButton.setVisible(false);
    storeSnapshotBButton.setVisible(false);
    compareSnapshotsButton.setVisible(false);

    homeButton.onClick = [this] { showPage(Page::home); };
    oscillatorButton.onClick = [this] { showPage(Page::oscillators); };
    ancientButton.onClick = [this] { showPage(Page::ancient); };
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
        compareSnapshotsButton.setButtonText(pas1::ui::utf8String("ПОКАЗАТЬ B"));
        compareSnapshotsButton.setToggleState(false, juce::dontSendNotification);
    };
    storeSnapshotBButton.onClick = [this] {
        snapshotB.setSize(0);
        processor_.getStateInformation(snapshotB);
        showingSnapshotB = true;
        compareSnapshotsButton.setButtonText(pas1::ui::utf8String("ПОКАЗАТЬ A"));
        compareSnapshotsButton.setToggleState(true, juce::dontSendNotification);
    };
    compareSnapshotsButton.setButtonText(pas1::ui::utf8String("ПОКАЗАТЬ B"));
    compareSnapshotsButton.onClick = [this] {
        auto& target = showingSnapshotB ? snapshotA : snapshotB;
        if (target.getSize() == 0) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon, pas1::ui::utf8String("Сравнение A/B — PAS 1"),
                pas1::ui::utf8String("Сначала сохраните снимки A и B, чтобы переключаться между ними."),
                pas1::ui::utf8String("ОК"), this);
            compareSnapshotsButton.setToggleState(showingSnapshotB,
                                                   juce::dontSendNotification);
            return;
        }
        processor_.setStateInformation(target.getData(),
                                       static_cast<int>(target.getSize()));
        showingSnapshotB = !showingSnapshotB;
        compareSnapshotsButton.setButtonText(showingSnapshotB ? pas1::ui::utf8String("ПОКАЗАТЬ A") : pas1::ui::utf8String("ПОКАЗАТЬ B"));
    };
    resetDefaultsButton.onClick = [this] {
        juce::Component::SafePointer<Pas1AudioProcessorEditor> safeThis(this);
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon, pas1::ui::utf8String("Сбросить параметры PAS 1?"),
            pas1::ui::utf8String("Все автоматизируемые параметры вернутся к заводским значениям, "
            "а настройки и снимки A/B в Morph будут очищены. "
            "Текущий звук изменится; сохранённые пользовательские пресеты не удаляются."),
            pas1::ui::utf8String("Сбросить"), pas1::ui::utf8String("Отмена"), this,
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
                safeThis->processor_.resetMorphConfiguration();
            }));
    };

    waveform.addItemList({pas1::ui::utf8String("Синус"), pas1::ui::utf8String("Треугольник"), pas1::ui::utf8String("Пила"), pas1::ui::utf8String("Прямоугольник")}, 1);
    waveform.setColour(juce::ComboBox::backgroundColourId, panel);
    waveform.setColour(juce::ComboBox::textColourId, text);
    waveform.setColour(juce::ComboBox::outlineColourId, brassTrim);
    waveform.setColour(juce::ComboBox::arrowColourId, accent);
    addAndMakeVisible(waveform);
    styleLabel(waveformLabel, pas1::ui::utf8String("ОСЦИЛЛЯТОР"));
    addAndMakeVisible(waveformLabel);

    for (auto* slider : {&attack, &decay, &sustain, &release, &master, &voices}) {
        styleSlider(*slider);
        addAndMakeVisible(*slider);
    }
    styleLabel(attackLabel, pas1::ui::utf8String("АТАКА"));
    styleLabel(decayLabel, pas1::ui::utf8String("СПАД"));
    styleLabel(sustainLabel, pas1::ui::utf8String("УДЕРЖАНИЕ"));
    styleLabel(releaseLabel, pas1::ui::utf8String("ЗАТУХАНИЕ"));
    styleLabel(masterLabel, pas1::ui::utf8String("ГРОМКОСТЬ"));
    styleLabel(voicesLabel, pas1::ui::utf8String("ГОЛОСА"));
    for (auto* label : {&attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &masterLabel, &voicesLabel})
        addAndMakeVisible(*label);

    voices.setNumDecimalPlacesToDisplay(0);
    voices.setTextValueSuffix(pas1::ui::utf8String(" нот"));
    attack.setTextValueSuffix(" s");
    decay.setTextValueSuffix(" s");
    release.setTextValueSuffix(" s");
    attack.setNumDecimalPlacesToDisplay(3);
    decay.setNumDecimalPlacesToDisplay(3);
    release.setNumDecimalPlacesToDisplay(3);

    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour{0xffeeeaf0});
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour{0xff34303b});
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, background);
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accent.withAlpha(0.45f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent.withAlpha(0.72f));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, muted);
    addAndMakeVisible(keyboard);

    auto& state = processor_.parameters;
    const auto setSliderDefault = [&state](const char* id, juce::Slider& slider) {
        if (auto* parameter = state.getParameter(id)) {
            slider.setDoubleClickReturnValue(
                true, parameter->convertFrom0to1(parameter->getDefaultValue()));
            slider.setTooltip(
                slider.getTooltip()
                + pas1::ui::utf8String(" Двойной щелчок возвращает заводское значение."));
        }
    };
    setSliderDefault("attack", attack);
    setSliderDefault("decay", decay);
    setSliderDefault("sustain", sustain);
    setSliderDefault("release", release);
    setSliderDefault("master", master);
    setSliderDefault("polyphony", voices);
    setSliderDefault("glidetime", glideTime);
    waveformAttachment = std::make_unique<ComboBoxAttachment>(state, "waveform", waveform);
    attackAttachment = std::make_unique<SliderAttachment>(state, "attack", attack);
    decayAttachment = std::make_unique<SliderAttachment>(state, "decay", decay);
    sustainAttachment = std::make_unique<SliderAttachment>(state, "sustain", sustain);
    releaseAttachment = std::make_unique<SliderAttachment>(state, "release", release);
    masterAttachment = std::make_unique<SliderAttachment>(state, "master", master);
    voicesAttachment = std::make_unique<SliderAttachment>(state, "polyphony", voices);

    voiceMode.addItemList({pas1::ui::utf8String("Полифония"), pas1::ui::utf8String("Монофония"), pas1::ui::utf8String("Легато")}, 1);
    voiceMode.setColour(juce::ComboBox::backgroundColourId, panel);
    voiceMode.setColour(juce::ComboBox::textColourId, text);
    voiceMode.setColour(juce::ComboBox::outlineColourId, brassTrim);
    voiceMode.setColour(juce::ComboBox::arrowColourId, accent);
    addAndMakeVisible(voiceMode);
    styleLabel(voiceModeLabel, pas1::ui::utf8String("РЕЖИМ ГОЛОСОВ"));
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
    glideTime.setColour(juce::Slider::textBoxOutlineColourId, brassTrim);
    addAndMakeVisible(glideTime);
    styleLabel(glideTimeLabel, pas1::ui::utf8String("ВРЕМЯ ГЛИССАНДО"));
    addAndMakeVisible(glideTimeLabel);
    voiceModeAttachment = std::make_unique<ComboBoxAttachment>(state, "voicemode", voiceMode);
    glideEnabledAttachment = std::make_unique<ButtonAttachment>(
        state, "glideenabled", glideEnabled);
    glideTimeAttachment = std::make_unique<SliderAttachment>(state, "glidetime", glideTime);
    uiReady = true;
    pas1::diagnostics::startupTrace("editor.setSize.before");
    setSize(1020, 740);
    pas1::diagnostics::startupTrace("editor.setSize.done");
    pas1::diagnostics::startupTrace("editor.showHome.before");
    showPage(Page::home);
    pas1::diagnostics::startupTrace("editor.showHome.done");
}

void Pas1AudioProcessorEditor::paint(juce::Graphics& graphics) {
    graphics.fillAll(background);

    const auto housing = getLocalBounds().toFloat().reduced(14.0f);
    juce::ColourGradient wood{juce::Colour{0xff50321f}, housing.getX(), housing.getY(),
                              darkWood, housing.getRight(), housing.getBottom(), false};
    graphics.setGradientFill(wood);
    graphics.fillRoundedRectangle(housing, 16.0f);
    graphics.setColour(accent.withAlpha(0.68f));
    graphics.drawRoundedRectangle(housing.reduced(1.0f), 15.0f, 1.2f);
    graphics.setColour(brassTrim.withAlpha(0.9f));
    graphics.drawRoundedRectangle(housing.reduced(5.0f), 12.0f, 0.8f);
    for (const auto x : {housing.getX() + 12.0f, housing.getRight() - 16.0f}) {
        for (const auto y : {housing.getY() + 12.0f, housing.getBottom() - 16.0f}) {
            graphics.setColour(accent.withAlpha(0.82f));
            graphics.fillEllipse(x, y, 4.0f, 4.0f);
            graphics.setColour(juce::Colour{0xfff4dcaa});
            graphics.fillEllipse(x + 1.0f, y + 1.0f, 1.4f, 1.4f);
        }
    }

    graphics.setColour(text);
    graphics.setFont(juce::FontOptions{32.0f, juce::Font::bold});
    graphics.drawText("PAS 1", 36, 18, 220, 40, juce::Justification::centredLeft);
    graphics.setColour(accent);
    graphics.setFont(juce::FontOptions{16.0f, juce::Font::bold});
    graphics.drawText("♪", 246, 22, 24, 30, juce::Justification::centred);
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions{9.0f, juce::Font::bold});
    graphics.drawText(pas1::ui::utf8String("ЗВУКОВАЯ ШКАТУЛКА"), 270, 30, 150, 14,
                      juce::Justification::centredLeft);
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions{11.0f});
    const auto subtitle = [this]() -> juce::String {
        switch (currentPage) {
            case Page::home: return pas1::ui::utf8String("СИНТЕЗАТОР-ШКАТУЛКА  /  ИСПОЛНЕНИЕ");
            case Page::oscillators: return pas1::ui::utf8String("ЗВУК  /  ОСЦИЛЛЯТОРЫ");
            case Page::ancient: return pas1::ui::utf8String("ДРЕВНИЕ ТЕМБРЫ  /  АУЛОС · ЛИРА · БРОНЗА");
            case Page::filters: return pas1::ui::utf8String("ЗВУК  /  ФИЛЬТРЫ И ОГИБАЮЩИЕ");
            case Page::modulation: return pas1::ui::utf8String("ЗВУК  /  МОДУЛЯЦИЯ");
            case Page::morph: return pas1::ui::utf8String("МАТРИЦА МОРФИНГА  /  АВТОМАТИЗАЦИЯ X–Y");
            case Page::generator: return pas1::ui::utf8String("РЕЦЕПТ ЗВУКА  /  ВАРИАЦИИ");
            case Page::soundDNA: return pas1::ui::utf8String("ДНК ЗВУКА  /  АНАЛИЗ ТЕМБРА");
            case Page::effects: return pas1::ui::utf8String("ЗВУК  /  ЭФФЕКТЫ");
            case Page::presets: return pas1::ui::utf8String("БИБЛИОТЕКА ПРЕСЕТОВ  /  ПОИСК И УПОРЯДОЧИВАНИЕ");
            case Page::settings: return pas1::ui::utf8String("ПЛАГИН  /  НАСТРОЙКИ И ИСПОЛНЕНИЕ");
            case Page::allParameters: return pas1::ui::utf8String("ДВИЖОК  /  ПОЛНЫЙ СПИСОК ПАРАМЕТРОВ");
        }
        return pas1::ui::utf8String("АДАПТИВНЫЙ СИНТЕЗАТОР");
    }();
    graphics.drawFittedText(subtitle, juce::Rectangle<int>{38, 56, 380, 18},
                            juce::Justification::centredLeft, 1, 0.7f);

    graphics.setFont(juce::FontOptions{9.0f, juce::Font::bold});
    graphics.setColour(muted);
    graphics.drawText("MIDI", 420, 40, 34, 12, juce::Justification::centredLeft);
    graphics.setColour(displayedMidiActivity > 0.06f ? accent : brassTrim);
    graphics.fillEllipse(451.0f, 41.0f, 9.0f, 9.0f);
    graphics.setColour(muted);
    graphics.drawText(pas1::ui::utf8String("ЗВУК"), 470, 40, 38, 12, juce::Justification::centredLeft);
    const juce::Rectangle<float> meterTrack{510.0f, 42.0f,
                                            static_cast<float>(std::max(80, getWidth() - 770)), 7.0f};
    graphics.setColour(brassTrim);
    graphics.fillRoundedRectangle(meterTrack, 3.5f);
    auto meterFill = meterTrack;
    meterFill.setWidth(meterFill.getWidth() * std::clamp(displayedAudioPeak, 0.0f, 1.0f));
    graphics.setColour(displayedAudioPeak > 0.92f ? juce::Colour{0xffd66c52} : accent);
    graphics.fillRoundedRectangle(meterFill, 3.5f);

    if (currentPage != Page::home) {
        graphics.setColour(panel);
        graphics.fillRoundedRectangle(30.0f, 116.0f, static_cast<float>(getWidth() - 60),
                                      static_cast<float>(getHeight() - 144), 12.0f);
        graphics.setColour(accent.withAlpha(0.3f));
        graphics.drawRoundedRectangle(30.5f, 116.5f, static_cast<float>(getWidth() - 61),
                                      static_cast<float>(getHeight() - 145), 11.5f, 1.0f);
        return;
    }

    graphics.setColour(accent.withAlpha(0.08f));
    graphics.fillEllipse(static_cast<float>(getWidth() - 290), -105.0f, 300.0f, 300.0f);

    graphics.setColour(panel);
    graphics.fillRoundedRectangle(30.0f, 120.0f, static_cast<float>(getWidth() - 60), 82.0f, 12.0f);
    graphics.fillRoundedRectangle(30.0f, 212.0f, static_cast<float>(getWidth() - 60), 176.0f, 12.0f);
    graphics.fillRoundedRectangle(30.0f, 398.0f, static_cast<float>(getWidth() - 60), 158.0f, 12.0f);
    graphics.setColour(accent.withAlpha(0.24f));
    graphics.drawRoundedRectangle(30.5f, 120.5f, static_cast<float>(getWidth() - 61), 81.0f, 11.5f, 0.8f);
    graphics.drawRoundedRectangle(30.5f, 212.5f, static_cast<float>(getWidth() - 61), 175.0f, 11.5f, 0.8f);
    graphics.drawRoundedRectangle(30.5f, 398.5f, static_cast<float>(getWidth() - 61), 157.0f, 11.5f, 0.8f);

    graphics.setColour(accent);
    graphics.setFont(juce::FontOptions{10.0f, juce::Font::bold});
    graphics.drawText(pas1::ui::utf8String("ИСТОЧНИК ЗВУКА"), 48, 130, 200, 16, juce::Justification::centredLeft);
    graphics.drawText(pas1::ui::utf8String("АМПЛИТУДНАЯ ОГИБАЮЩАЯ"), 48, 222, 250, 16, juce::Justification::centredLeft);
    graphics.drawText(pas1::ui::utf8String("ИСПОЛНЕНИЕ"), 48, 408, 100, 16, juce::Justification::centredLeft);

    const auto readValue = [this](const char* id, float fallback) {
        if (const auto* value = processor_.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);
        return fallback;
    };
    const auto waveLeft = 400.0f;
    const auto waveRight = static_cast<float>(std::min(740, getWidth() - 250));
    const auto waveTop = 145.0f;
    const auto waveBottom = 180.0f;
    graphics.setColour(brassTrim);
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

    const auto attackSeconds = std::max(0.001f, readValue("attack", 0.01f));
    const auto decaySeconds = std::max(0.001f, readValue("decay", 0.15f));
    const auto releaseSeconds = std::max(0.001f, readValue("release", 0.2f));
    const auto sustainLevel = std::clamp(readValue("sustain", 0.75f), 0.0f, 1.0f);
    const auto plot = makeEnvelopePlot(getWidth(), attackSeconds, decaySeconds,
                                       sustainLevel, releaseSeconds);
    graphics.setColour(muted);
    graphics.setFont(juce::FontOptions{9.0f});
    graphics.drawText(pas1::ui::utf8String("ПЕРЕТАЩИТЕ ТОЧКИ, ЧТОБЫ ИЗМЕНИТЬ ОГИБАЮЩУЮ"),
                      static_cast<int>(plot.left), 232,
                      static_cast<int>(plot.right - plot.left), 15,
                      juce::Justification::centredLeft);
    graphics.setColour(brassTrim);
    for (int line = 1; line <= 3; ++line) {
        const auto y = plot.top + (plot.bottom - plot.top) * static_cast<float>(line) / 4.0f;
        graphics.drawLine(plot.left, y, plot.right, y, 1.0f);
    }
    juce::Path envelope;
    envelope.startNewSubPath(plot.left, plot.bottom);
    envelope.lineTo(plot.attackX, plot.top);
    envelope.lineTo(plot.decayX, plot.sustainY);
    envelope.lineTo(plot.releaseX, plot.sustainY);
    envelope.lineTo(plot.right, plot.bottom);
    graphics.setColour(accent);
    graphics.strokePath(envelope, juce::PathStrokeType{2.0f});

    const auto drawHandle = [&graphics](float x, float y) {
        graphics.setColour(accent.withAlpha(0.24f));
        graphics.fillEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f);
        graphics.setColour(background);
        graphics.fillEllipse(x - 4.5f, y - 4.5f, 9.0f, 9.0f);
        graphics.setColour(accent);
        graphics.drawEllipse(x - 4.5f, y - 4.5f, 9.0f, 9.0f, 1.5f);
    };
    drawHandle(plot.attackX, plot.top);
    drawHandle(plot.decayX, plot.sustainY);
    drawHandle(plot.releaseX, plot.sustainY);
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
    if (currentPage == Page::home) {
        // These plots are painted by the editor itself, not by the child controls.
        // Refresh them as host automation and the attached controls change parameters.
        repaint(30, 120, getWidth() - 60, 270);
        spectrumAnalyzer->update();
    } else if (currentPage == Page::oscillators) {
        oscillatorWaveformGraph->update();
    } else if (currentPage == Page::filters) {
        filterGraphs->update();
    }
}

void Pas1AudioProcessorEditor::mouseDown(const juce::MouseEvent& event) {
    if (currentPage != Page::home)
        return;

    const auto readValue = [this](const char* id, float fallback) {
        if (const auto* value = processor_.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);
        return fallback;
    };
    const auto plot = makeEnvelopePlot(
        getWidth(), std::max(0.001f, readValue("attack", 0.01f)),
        std::max(0.001f, readValue("decay", 0.15f)),
        std::clamp(readValue("sustain", 0.75f), 0.0f, 1.0f),
        std::max(0.001f, readValue("release", 0.2f)));
    const std::array<std::pair<EnvelopeHandle, juce::Point<float>>, 3> handles{{
        {EnvelopeHandle::attack, {plot.attackX, plot.top}},
        {EnvelopeHandle::decaySustain, {plot.decayX, plot.sustainY}},
        {EnvelopeHandle::release, {plot.releaseX, plot.sustainY}}
    }};

    auto closest = EnvelopeHandle::none;
    auto closestDistanceSquared = 18.0f * 18.0f;
    for (const auto& [handle, point] : handles) {
        const auto distance = event.position - point;
        const auto distanceSquared = distance.x * distance.x + distance.y * distance.y;
        if (distanceSquared < closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closest = handle;
        }
    }
    if (closest == EnvelopeHandle::none)
        return;

    endEnvelopeGesture();
    activeEnvelopeHandle = closest;
    const auto beginGesture = [this](const char* id) {
        if (auto* parameter = processor_.parameters.getParameter(id)) {
            parameter->beginChangeGesture();
            envelopeGestureParameters[envelopeGestureParameterCount++] = parameter;
        }
    };
    if (closest == EnvelopeHandle::attack) {
        beginGesture("attack");
    } else if (closest == EnvelopeHandle::decaySustain) {
        beginGesture("decay");
        beginGesture("sustain");
    } else {
        beginGesture("release");
    }
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void Pas1AudioProcessorEditor::mouseDrag(const juce::MouseEvent& event) {
    if (activeEnvelopeHandle != EnvelopeHandle::none)
        updateEnvelopeFromGraph(event.position);
}

void Pas1AudioProcessorEditor::mouseUp(const juce::MouseEvent&) {
    endEnvelopeGesture();
}

void Pas1AudioProcessorEditor::updateEnvelopeFromGraph(juce::Point<float> position) {
    const auto readValue = [this](const char* id, float fallback) {
        if (const auto* value = processor_.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);
        return fallback;
    };
    const auto attackSeconds = std::max(0.001f, readValue("attack", 0.01f));
    const auto decaySeconds = std::max(0.001f, readValue("decay", 0.15f));
    const auto releaseSeconds = std::max(0.001f, readValue("release", 0.2f));
    const auto plot = makeEnvelopePlot(getWidth(), attackSeconds, decaySeconds,
                                       readValue("sustain", 0.75f), releaseSeconds);
    const auto width = std::max(1.0f, plot.right - plot.left);
    const auto fraction = std::clamp((position.x - plot.left) / width, 0.02f, 0.98f);

    const auto setValue = [this](const char* id, float value) {
        if (auto* parameter = processor_.parameters.getParameter(id)) {
            const auto range = parameter->getNormalisableRange();
            const auto bounded = std::clamp(value, range.start, range.end);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(bounded));
        }
    };

    switch (activeEnvelopeHandle) {
        case EnvelopeHandle::attack: {
            const auto timedFraction = std::clamp(
                fraction / (1.0f - plot.holdFraction), 0.02f, 0.98f);
            const auto remaining = decaySeconds + releaseSeconds;
            setValue("attack", timedFraction * remaining / (1.0f - timedFraction));
            break;
        }
        case EnvelopeHandle::decaySustain: {
            const auto timedFraction = std::clamp(
                fraction / (1.0f - plot.holdFraction), 0.02f, 0.98f);
            const auto denominator = std::max(0.02f, 1.0f - timedFraction);
            const auto nextDecay =
                (timedFraction * (attackSeconds + releaseSeconds) - attackSeconds)
                / denominator;
            const auto sustain = std::clamp(
                (plot.bottom - position.y) / (plot.bottom - plot.top), 0.0f, 1.0f);
            setValue("decay", nextDecay);
            setValue("sustain", sustain);
            break;
        }
        case EnvelopeHandle::release: {
            const auto timedFraction = std::clamp(
                (fraction - plot.holdFraction) / (1.0f - plot.holdFraction),
                0.02f, 0.98f);
            const auto nextRelease =
                (attackSeconds + decaySeconds) * (1.0f - timedFraction) / timedFraction;
            setValue("release", nextRelease);
            break;
        }
        case EnvelopeHandle::none:
            break;
    }
    repaint(30, 212, getWidth() - 60, 176);
}

void Pas1AudioProcessorEditor::endEnvelopeGesture() {
    for (std::size_t index = 0; index < envelopeGestureParameterCount; ++index) {
        if (auto* parameter = envelopeGestureParameters[index])
            parameter->endChangeGesture();
        envelopeGestureParameters[index] = nullptr;
    }
    envelopeGestureParameterCount = 0;
    activeEnvelopeHandle = EnvelopeHandle::none;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void Pas1AudioProcessorEditor::resized() {
    pas1::diagnostics::startupTrace("editor.resized.begin");
    if (!uiReady) {
        pas1::diagnostics::startupTrace("editor.resized.skipped_not_ready");
        return;
    }
    const auto width = getWidth();
    const std::array<std::pair<juce::TextButton*, int>, 11> tabs{{
        {&homeButton, 78}, {&oscillatorButton, 58}, {&ancientButton, 80}, {&filterButton, 72},
        {&modulationButton, 94}, {&morphButton, 82}, {&generatorButton, 92},
        {&soundDNAButton, 94}, {&effectsButton, 82}, {&presetButton, 82},
        {&settingsButton, 82}
    }};
    auto tabX = 30;
    std::size_t tabIndex = 0;
    for (const auto& [button, tabWidth] : tabs) {
        char marker[64]{};
        std::snprintf(marker, sizeof(marker), "editor.resized.tab.%zu.before", tabIndex);
        pas1::diagnostics::startupTrace(marker);
        button->setBounds(tabX, 76, tabWidth, 29);
        std::snprintf(marker, sizeof(marker), "editor.resized.tab.%zu.after", tabIndex);
        pas1::diagnostics::startupTrace(marker);
        tabX += tabWidth + 4;
        ++tabIndex;
    }
    pas1::diagnostics::startupTrace("editor.resized.headerButtons.before");
    resetDefaultsButton.setBounds(width - 348, 20, 62, 30);
    savePresetButton.setBounds(width - 278, 20, 96, 30);
    engineButton.setBounds(width - 174, 20, 138, 30);

    const auto pageBounds = juce::Rectangle<int>{30, 116, width - 60, getHeight() - 142};
    const std::array<std::pair<const char*, juce::Component*>, 11> pages{{
        {"oscillatorPage", oscillatorPage.get()},
        {"ancientPage", ancientPage.get()},
        {"filterPage", filterPage.get()},
        {"effectsPage", effectsPage.get()},
        {"settingsPage", settingsPage.get()},
        {"generatorPanel", generatorPanel.get()},
        {"morphPanel", morphPanel.get()},
        {"soundDNAPanel", soundDNAPanel.get()},
        {"presetsPanel", presetsPanel.get()},
        {"modulationTabs", modulationTabs.get()},
        {"engineEditor", engineEditor.get()}
    }};
    for (const auto& [name, page] : pages) {
        const auto before = std::string{"editor.resized.page."} + name + ".before";
        pas1::diagnostics::startupTrace(before);
        page->setBounds(pageBounds);
        const auto after = std::string{"editor.resized.page."} + name + ".after";
        pas1::diagnostics::startupTrace(after);
    }
    pas1::diagnostics::startupTrace("editor.resized.settingsControls.before");
    storeSnapshotAButton.setBounds(46, 121, 72, 27);
    storeSnapshotBButton.setBounds(124, 121, 72, 27);
    compareSnapshotsButton.setBounds(202, 121, 92, 27);
    settingsPage->setBounds(pageBounds.withY(152).withHeight(pageBounds.getHeight() - 36));

    pas1::diagnostics::startupTrace("editor.resized.waveform.before");
    waveformLabel.setBounds(50, 145, 125, 18);
    waveform.setBounds(180, 140, 190, 34);

    const auto sliderWidth = 104;
    const auto gap = 18;
    auto x = 48;
    const auto sliderY = 253;
    pas1::diagnostics::startupTrace("editor.resized.envelopeSliders.before");
    for (auto* item : {&attack, &decay, &sustain, &release}) {
        item->setBounds(x, sliderY, sliderWidth, 112);
        x += sliderWidth + gap;
    }
    attackLabel.setBounds(attack.getX(), 234, sliderWidth, 18);
    decayLabel.setBounds(decay.getX(), 234, sliderWidth, 18);
    sustainLabel.setBounds(sustain.getX(), 234, sliderWidth, 18);
    releaseLabel.setBounds(release.getX(), 234, sliderWidth, 18);

    const auto rightControlsX = std::max(520, width - 230);
    pas1::diagnostics::startupTrace("editor.resized.performanceControls.before");
    master.setBounds(rightControlsX, 137, 92, 55);
    voices.setBounds(rightControlsX + 104, 137, 92, 55);
    masterLabel.setBounds(master.getX(), 122, 92, 18);
    voicesLabel.setBounds(voices.getX(), 122, 92, 18);
    voiceModeLabel.setBounds(48, 422, 144, 14);
    voiceMode.setBounds(48, 438, 144, 29);
    glideEnabled.setBounds(204, 438, 86, 29);
    glideTimeLabel.setBounds(304, 422, 160, 14);
    glideTime.setBounds(304, 436, 210, 34);
    pas1::diagnostics::startupTrace("editor.resized.keyboard.before");
    keyboard.setBounds(48, 526, width - 96, 54);
    pas1::diagnostics::startupTrace("editor.resized.spectrumAnalyzer.before");
    spectrumAnalyzer->setBounds(540, 412, width - 594, 108);
    pas1::diagnostics::startupTrace("editor.resized.done");
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
    pas1::diagnostics::startupTrace("editor.showPage.begin");
    currentPage = page;
    if (page == Page::oscillators)
        oscillatorWaveformGraph->update();
    else if (page == Page::filters)
        filterGraphs->update();
    const auto home = page == Page::home;
    engineEditor->setVisible(page == Page::allParameters);
    oscillatorPage->setVisible(page == Page::oscillators);
    ancientPage->setVisible(page == Page::ancient);
    filterPage->setVisible(page == Page::filters);
    modulationTabs->setVisible(page == Page::modulation);
    morphPanel->setVisible(page == Page::morph);
    generatorPanel->setVisible(page == Page::generator);
    soundDNAPanel->setVisible(page == Page::soundDNA);
    effectsPage->setVisible(page == Page::effects);
    presetsPanel->setVisible(page == Page::presets);
    settingsPage->setVisible(page == Page::settings);
    spectrumAnalyzer->setVisible(home);
    storeSnapshotAButton.setVisible(page == Page::settings);
    storeSnapshotBButton.setVisible(page == Page::settings);
    compareSnapshotsButton.setVisible(page == Page::settings);
    setCoreControlsVisible(home);

    homeButton.setToggleState(page == Page::home, juce::dontSendNotification);
    oscillatorButton.setToggleState(page == Page::oscillators, juce::dontSendNotification);
    ancientButton.setToggleState(page == Page::ancient, juce::dontSendNotification);
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

    pas1::diagnostics::startupTrace("editor.showPage.resized.before");
    resized();
    pas1::diagnostics::startupTrace("editor.showPage.resized.done");
    repaint();
}

void Pas1AudioProcessorEditor::savePreset() {
    auto* alert = new juce::AlertWindow(pas1::ui::utf8String("Сохранить пресет PAS 1"),
                                        pas1::ui::utf8String("Сохраните текущий звук в библиотеку пресетов."),
                                        juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("presetName", pas1::ui::utf8String("Мой пресет"), pas1::ui::utf8String("Название"));
    alert->addTextEditor("presetCategory", pas1::ui::utf8String("Пользовательские"), pas1::ui::utf8String("Категория"));
    alert->addButton(pas1::ui::utf8String("Сохранить"), 1, juce::KeyPress{juce::KeyPress::returnKey});
    alert->addButton(pas1::ui::utf8String("Отмена"), 0, juce::KeyPress{juce::KeyPress::escapeKey});
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
        juce::MessageBoxIconType::WarningIcon, pas1::ui::utf8String("Пресеты PAS 1"), message,
        pas1::ui::utf8String("ОК"), this);
}
