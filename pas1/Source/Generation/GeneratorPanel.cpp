#include "GeneratorPanel.h"
#include "UI/Utf8Text.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>

namespace {
const juce::Colour panelBackground{0xff302015};
const juce::Colour panelAccent{0xffd8b36a};
const juce::Colour panelText{0xfff4e8cf};
const juce::Colour panelMuted{0xffbba483};
const std::array<const char*, 10> serializedVariationModes{
    "Full Random", "Subtle", "Aggressive", "Genre", "Morph", "Texture",
    "Preset Mutation", "Generate Similar", "Surprise Me", "Seeded"};
const std::array<const char*, 10> displayedVariationModes{
    "Случайный", "Мягкая вариация", "Радикальная", "По жанру",
    "Морфинг", "Текстура", "Модификация", "Похожий звук", "Сюрприз", "По зерну"};

void setupSlider(juce::Slider& slider) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 18);
    slider.setRange(0.0, 1.0, 0.01);
    slider.setValue(0.5, juce::dontSendNotification);
    slider.setColour(juce::Slider::rotarySliderFillColourId, panelAccent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour{0xff745332});
    slider.setColour(juce::Slider::thumbColourId, panelText);
    slider.setColour(juce::Slider::textBoxTextColourId, panelText);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, panelBackground);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void setupButton(juce::TextButton& button) {
    button.setColour(juce::TextButton::buttonColourId, panelBackground);
    button.setColour(juce::TextButton::textColourOffId, panelText);
    button.setColour(juce::TextButton::textColourOnId, panelAccent);
}
} // namespace

GeneratorPanel::GeneratorPanel(Pas1AudioProcessor& processor) : processor_(processor) {
    recipeText.setMultiLine(true);
    recipeText.setReturnKeyStartsNewLine(true);
    recipeText.setTextToShowWhenEmpty(
        pas1::ui::utf8String("Опишите звук, например: тёплый подвижный аналоговый бас с металлической атакой и длинным широким хвостом."),
        panelMuted);
    recipeText.setColour(juce::TextEditor::backgroundColourId, panelBackground);
    recipeText.setColour(juce::TextEditor::textColourId, panelText);
    recipeText.setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff745332});
    addAndMakeVisible(recipeText);

    for (std::size_t index = 0; index < displayedVariationModes.size(); ++index)
        variationMode.addItem(pas1::ui::utf8String(displayedVariationModes[index]),
                              static_cast<int>(index) + 1);
    variationMode.setSelectedId(2, juce::dontSendNotification);
    variationMode.setColour(juce::ComboBox::backgroundColourId, panelBackground);
    variationMode.setColour(juce::ComboBox::textColourId, panelText);
    variationMode.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff745332});
    variationMode.setTooltip(pas1::ui::utf8String("Выберите способ создания вариации. Режим «По зерну» позволяет повторить результат по заданному числу."));
    addAndMakeVisible(variationMode);

    variationAmount.setSliderStyle(juce::Slider::LinearHorizontal);
    variationAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 22);
    variationAmount.setRange(0.0, 100.0, 1.0);
    variationAmount.setTextValueSuffix(" %");
    variationAmount.setValue(35.0, juce::dontSendNotification);
    variationAmount.setTextValueSuffix("%");
    variationAmount.setColour(juce::Slider::trackColourId, panelAccent);
    variationAmount.setColour(juce::Slider::thumbColourId, panelText);
    variationAmount.setColour(juce::Slider::textBoxTextColourId, panelText);
    variationAmount.setColour(juce::Slider::textBoxBackgroundColourId, panelBackground);
    addAndMakeVisible(variationAmount);

    seedText.setText("1", juce::dontSendNotification);
    seedText.setColour(juce::TextEditor::backgroundColourId, panelBackground);
    seedText.setColour(juce::TextEditor::textColourId, panelText);
    seedText.setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff745332});
    seedText.setTooltip(pas1::ui::utf8String("Введите то же зерно генерации, чтобы повторить вариацию."));
    seedText.setInputRestrictions(10, "0123456789");
    addAndMakeVisible(seedText);
    lockedParameters.setText("master, polyphony", juce::dontSendNotification);
    lockedParameters.setColour(juce::TextEditor::backgroundColourId, panelBackground);
    lockedParameters.setColour(juce::TextEditor::textColourId, panelText);
    lockedParameters.setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff745332});
    lockedParameters.setTooltip(pas1::ui::utf8String("Перечислите через запятую ID параметров, которые генератор не должен менять."));
    addAndMakeVisible(lockedParameters);
    const std::array<juce::String, 4> sectionTooltips{
        pas1::ui::utf8String("Не менять настройки осцилляторов, унисона, волновой таблицы, саб-низа, шума и музыкальной шкатулки."),
        pas1::ui::utf8String("Не менять фильтр и амплитудную/фильтровую огибающие."),
        pas1::ui::utf8String("Не менять низкочастотные генераторы и матрицу модуляции."),
        pas1::ui::utf8String("Не менять искажение, хорус, фазер, задержку, реверберацию, эквалайзер и ширину стерео.")};
    for (std::size_t index = 0; index < sectionLockButtons.size(); ++index) {
        auto& button = sectionLockButtons[index];
        setupButton(button);
        button.setClickingTogglesState(true);
        button.setColour(juce::TextButton::buttonOnColourId,
                         panelAccent.withAlpha(0.24f));
        button.setTooltip(sectionTooltips[index]);
        button.onClick = [this] {
            const auto activeLocks = std::count_if(
                sectionLockButtons.begin(), sectionLockButtons.end(),
                [](const auto& lockButton) { return lockButton.getToggleState(); });
            statusLabel.setText(
                activeLocks == 0
                    ? pas1::ui::utf8String("Блокировки разделов выключены; действуют только дополнительные ID параметров.")
                    : juce::String(activeLocks)
                          + pas1::ui::utf8String(" блокировок разделов действует при генерации."),
                juce::dontSendNotification);
        };
        addAndMakeVisible(button);
    }

    variationSelector.setTextWhenNothingSelected(pas1::ui::utf8String("Выберите вариацию"));
    variationSelector.setColour(juce::ComboBox::backgroundColourId, panelBackground);
    variationSelector.setColour(juce::ComboBox::textColourId, panelText);
    variationSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff745332});
    variationSelector.setTooltip(pas1::ui::utf8String("Для каждой вариации показано зерно генерации. Сравните звук кнопкой «До / После»."));
    variationSelector.onChange = [this] {
        if (variationSelector.getSelectedId() > 0)
            selectVariation(variationSelector.getSelectedId() - 1);
    };
    addAndMakeVisible(variationSelector);

    for (std::size_t index = 0; index < controls.size(); ++index) {
        setupSlider(controls[index]);
        addAndMakeVisible(controls[index]);
        controlLabels[index].setText(controlNames[index], juce::dontSendNotification);
        controlLabels[index].setJustificationType(juce::Justification::centred);
        controlLabels[index].setColour(juce::Label::textColourId, panelMuted);
        addAndMakeVisible(controlLabels[index]);
    }

    const auto addButton = [this](juce::TextButton& button) {
        setupButton(button);
        addAndMakeVisible(button);
    };
    for (auto* button : {&recipeButton, &variationButton, &generateFourButton,
                         &generateSixteenButton, &beforeAfterButton, &undoButton, &redoButton,
                         &keepButton, &revertButton, &applyButton, &saveRecipeButton,
                         &loadRecipeButton})
        addButton(*button);

    recipeButton.onClick = [this] { previewRecipe(); };
    variationButton.onClick = [this] { previewVariation(); };
    generateFourButton.onClick = [this] { generateVariations(4); };
    generateSixteenButton.onClick = [this] { generateVariations(16); };
    beforeAfterButton.onClick = [this] { showBeforeAfter(); };
    undoButton.onClick = [this] { undo(); };
    redoButton.onClick = [this] { redo(); };
    keepButton.onClick = [this] { keepCurrent(); };
    revertButton.onClick = [this] { revert(); };
    applyButton.onClick = [this] {
        afterState = processor_.parameters.copyState();
        originalState = afterState.createCopy();
        previewTransaction = false;
        showingBefore = false;
        undoStates.clear();
        redoStates.clear();
        statusLabel.setText(pas1::ui::utf8String("Применено. Продолжайте настройку или сохраните звук как пресет."),
                            juce::dontSendNotification);
    };
    saveRecipeButton.onClick = [this] { saveRecipe(); };
    loadRecipeButton.onClick = [this] { loadRecipe(); };
    recipeButton.setTooltip(pas1::ui::utf8String("Создать и прослушать звук по текстовому описанию."));

    for (auto* pair : {&amountLabel, &modeLabel, &seedLabel, &lockedLabel, &variationLabel})
        pair->setColour(juce::Label::textColourId, panelMuted);
    amountLabel.setText(pas1::ui::utf8String("СИЛА ВАРИАЦИИ"), juce::dontSendNotification);
    modeLabel.setText(pas1::ui::utf8String("РЕЖИМ"), juce::dontSendNotification);
    seedLabel.setText(pas1::ui::utf8String("ЗЕРНО"), juce::dontSendNotification);
    lockedLabel.setText(pas1::ui::utf8String("БЛОКИРОВКИ · ДОП. ID"), juce::dontSendNotification);
    variationLabel.setText(pas1::ui::utf8String("НАБОР ВАРИАЦИЙ"), juce::dontSendNotification);
    for (auto* label : {&amountLabel, &modeLabel, &seedLabel, &lockedLabel, &variationLabel})
        addAndMakeVisible(*label);

    statusLabel.setColour(juce::Label::textColourId, panelMuted);
    statusLabel.setText(pas1::ui::utf8String("Локальный генератор по правилам — без ИИ-модели и подключения к сети."),
                        juce::dontSendNotification);
    addAndMakeVisible(statusLabel);
}

void GeneratorPanel::paint(juce::Graphics& graphics) {
    graphics.setColour(panelBackground);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
}

void GeneratorPanel::resized() {
    const auto width = getWidth();
    recipeText.setBounds(12, 14, width - 24, 58);

    const auto cellWidth = (width - 40) / 3;
    for (std::size_t index = 0; index < controls.size(); ++index) {
        const auto row = static_cast<int>(index / 3);
        const auto column = static_cast<int>(index % 3);
        const auto x = 12 + column * cellWidth;
        const auto y = 82 + row * 90;
        controlLabels[index].setBounds(x, y, cellWidth, 18);
        controls[index].setBounds(x + (cellWidth - 88) / 2, y + 16, 88, 72);
    }

    const auto controlY = 358;
    amountLabel.setBounds(16, controlY, 136, 18);
    variationAmount.setBounds(16, controlY + 16, 190, 28);
    modeLabel.setBounds(216, controlY, 116, 18);
    variationMode.setBounds(216, controlY + 16, 150, 28);
    seedLabel.setBounds(376, controlY, 72, 18);
    seedText.setBounds(376, controlY + 16, 88, 28);
    variationLabel.setBounds(474, controlY, 142, 18);
    variationSelector.setBounds(474, controlY + 16, 170, 28);
    const auto lockAreaWidth = std::max(0, width - 666);
    lockedLabel.setBounds(654, controlY, lockAreaWidth, 18);
    const auto lockButtonGap = 4;
    const auto lockButtonWidth = std::max(
        38, (lockAreaWidth - lockButtonGap * static_cast<int>(sectionLockButtons.size() - 1))
                / static_cast<int>(sectionLockButtons.size()));
    for (std::size_t index = 0; index < sectionLockButtons.size(); ++index)
        sectionLockButtons[index].setBounds(
            654 + static_cast<int>(index) * (lockButtonWidth + lockButtonGap),
            controlY + 16, lockButtonWidth, 22);
    lockedParameters.setBounds(654, controlY + 41, lockAreaWidth, 23);

    const auto buttonY = 440;
    const auto buttonGap = 8;
    const auto buttonWidth = std::max(62, (width - 40 - 6 * buttonGap) / 7);
    juce::TextButton* firstRow[]{
        &recipeButton, &variationButton, &generateFourButton, &generateSixteenButton,
        &beforeAfterButton, &undoButton, &redoButton};
    for (std::size_t index = 0; index < std::size(firstRow); ++index)
        firstRow[index]->setBounds(12 + static_cast<int>(index) * (buttonWidth + buttonGap),
                                   buttonY, buttonWidth, 30);

    juce::TextButton* secondRow[]{
        &keepButton, &revertButton, &applyButton, &saveRecipeButton, &loadRecipeButton};
    const auto secondWidth = std::max(100, (width - 56) / 5);
    for (std::size_t index = 0; index < std::size(secondRow); ++index)
        secondRow[index]->setBounds(12 + static_cast<int>(index) * (secondWidth + 8),
                                    buttonY + 38, secondWidth, 30);
    statusLabel.setBounds(14, buttonY + 74, width - 28, 22);
}

GeneratorPanel::ParameterList GeneratorPanel::captureParameters() const {
    ParameterList result;
    const auto state = processor_.parameters.copyState();
    result.reserve(static_cast<std::size_t>(state.getNumChildren()));
    for (int index = 0; index < state.getNumChildren(); ++index) {
        const auto node = state.getChild(index);
        if (!node.hasProperty("id"))
            continue;
        const auto id = node.getProperty("id").toString();
        const auto* parameter = processor_.parameters.getParameter(id);
        if (parameter == nullptr)
            continue;
        const auto& range = parameter->getNormalisableRange();
        const auto discrete = dynamic_cast<const juce::AudioParameterChoice*>(parameter) != nullptr
                              || dynamic_cast<const juce::AudioParameterInt*>(parameter) != nullptr
                              || dynamic_cast<const juce::AudioParameterBool*>(parameter) != nullptr;
        result.push_back({id.toStdString(), parameter->getValue(), parameter->getDefaultValue(),
                          range.start, range.end, range.skew, parameter->getNumSteps(), discrete});
    }
    return result;
}

juce::ValueTree GeneratorPanel::makeSnapshot(const juce::ValueTree& base,
                                             const ParameterList& values) const {
    auto snapshot = base.createCopy();
    for (int index = 0; index < snapshot.getNumChildren(); ++index) {
        auto node = snapshot.getChild(index);
        if (!node.hasProperty("id"))
            continue;
        const auto id = node.getProperty("id").toString();
        const auto found = std::find_if(values.begin(), values.end(),
                                        [&id](const auto& value) { return value.id == id.toStdString(); });
        if (found == values.end())
            continue;
        if (const auto* parameter = processor_.parameters.getParameter(id))
            node.setProperty("value", parameter->convertFrom0to1(found->normalizedValue), nullptr);
    }
    return snapshot;
}

void GeneratorPanel::applyParameters(const ParameterList& values) {
    for (const auto& value : values)
        if (auto* parameter = processor_.parameters.getParameter(value.id))
            parameter->setValueNotifyingHost(value.normalizedValue);
    afterState = processor_.parameters.copyState();
}

void GeneratorPanel::applyState(const juce::ValueTree& state) {
    if (state.isValid())
        processor_.parameters.replaceState(state.createCopy());
    showingBefore = false;
}

void GeneratorPanel::beginPreview() {
    if (previewTransaction)
        return;
    originalState = processor_.parameters.copyState();
    afterState = originalState.createCopy();
    undoStates.clear();
    redoStates.clear();
    previewTransaction = true;
    showingBefore = false;
}

void GeneratorPanel::previewRecipe() {
    beginPreview();
    pushUndoState();
    const auto result = pas1::generation::SoundGenerator::createRecipe(
        captureParameters(), recipeText.getText().toStdString(), getRecipeControls(),
        getLockedParameters());
    applyParameters(result);
    statusLabel.setText(
        pas1::ui::utf8String("Прослушивание рецепта. Нажмите «Применить», чтобы оставить звук, или «Вернуть», чтобы восстановить прежний."),
        juce::dontSendNotification);
}

void GeneratorPanel::previewVariation() {
    beginPreview();
    pushUndoState();
    auto seed = getSeed();
    if (getVariationMode() == pas1::generation::VariationMode::surprise) {
        seed = static_cast<std::uint32_t>(juce::Random::getSystemRandom().nextInt());
        seedText.setText(juce::String(static_cast<juce::int64>(seed)), juce::dontSendNotification);
    }
    const auto result = pas1::generation::SoundGenerator::createVariation(
        captureParameters(), getVariationMode(),
        static_cast<float>(variationAmount.getValue() / 100.0), seed,
        getLockedParameters(), recipeText.getText().toStdString());
    applyParameters(result);
    statusLabel.setText(pas1::ui::utf8String("Прослушивание вариации — зерно ")
                            + juce::String(static_cast<juce::int64>(seed))
                            + pas1::ui::utf8String(". Введите это зерно снова, чтобы повторить результат."),
                        juce::dontSendNotification);
}

void GeneratorPanel::generateVariations(int count) {
    beginPreview();
    variations.clear();
    variationSeeds.clear();
    variationSelector.clear(juce::dontSendNotification);
    const auto baseState = processor_.parameters.copyState();
    const auto base = captureParameters();
    auto seed = getSeed();
    if (getVariationMode() == pas1::generation::VariationMode::surprise) {
        seed = static_cast<std::uint32_t>(juce::Random::getSystemRandom().nextInt());
        seedText.setText(juce::String(static_cast<juce::int64>(seed)), juce::dontSendNotification);
    }
    if (seed == 0)
        seed = 0x6d2b79f5u;
    seedText.setText(juce::String(static_cast<juce::int64>(seed)),
                     juce::dontSendNotification);

    for (int index = 0; index < count; ++index) {
        const auto variationSeed =
            seed + static_cast<std::uint32_t>(index) * 2654435761u;
        const auto result = pas1::generation::SoundGenerator::createVariation(
            base, getVariationMode(), static_cast<float>(variationAmount.getValue() / 100.0),
            variationSeed,
            getLockedParameters(), recipeText.getText().toStdString());
        variations.push_back(makeSnapshot(baseState, result));
        variationSeeds.push_back(variationSeed);
        variationSelector.addItem(
            pas1::ui::utf8String("Вариация ") + juce::String(index + 1)
                + pas1::ui::utf8String(" — зерно ")
                + juce::String(static_cast<juce::int64>(variationSeed)),
            index + 1);
    }

    if (!variations.empty()) {
        variationSelector.setSelectedId(1, juce::sendNotification);
        statusLabel.setText(juce::String(count)
                                + pas1::ui::utf8String(" вариаций создано. Выберите одну, чтобы прослушать или сравнить."),
                            juce::dontSendNotification);
    }
}

void GeneratorPanel::selectVariation(int index) {
    if (index < 0 || index >= static_cast<int>(variations.size()))
        return;
    pushUndoState();
    applyState(variations[static_cast<std::size_t>(index)]);
    if (index < static_cast<int>(variationSeeds.size()))
        seedText.setText(
            juce::String(static_cast<juce::int64>(
                variationSeeds[static_cast<std::size_t>(index)])),
            juce::dontSendNotification);
    afterState = processor_.parameters.copyState();
    auto status = pas1::ui::utf8String("Прослушивание вариации ") + juce::String(index + 1);
    if (index < static_cast<int>(variationSeeds.size()))
        status += pas1::ui::utf8String(" — зерно ")
                  + juce::String(static_cast<juce::int64>(
                      variationSeeds[static_cast<std::size_t>(index)]));
    statusLabel.setText(status
                            + pas1::ui::utf8String(". Нажмите «Применить», чтобы оставить звук, или «Вернуть», чтобы восстановить прежний."),
                        juce::dontSendNotification);
}

void GeneratorPanel::pushUndoState() {
    undoStates.push_back(processor_.parameters.copyState());
    redoStates.clear();
}

void GeneratorPanel::undo() {
    if (undoStates.empty())
        return;
    redoStates.push_back(processor_.parameters.copyState());
    auto state = undoStates.back();
    undoStates.pop_back();
    applyState(state);
    afterState = processor_.parameters.copyState();
}

void GeneratorPanel::redo() {
    if (redoStates.empty())
        return;
    undoStates.push_back(processor_.parameters.copyState());
    auto state = redoStates.back();
    redoStates.pop_back();
    applyState(state);
    afterState = processor_.parameters.copyState();
}

void GeneratorPanel::showBeforeAfter() {
    if (!previewTransaction || !originalState.isValid())
        return;
    if (showingBefore) {
        applyState(afterState);
        showingBefore = false;
        statusLabel.setText(pas1::ui::utf8String("Показан созданный звук."), juce::dontSendNotification);
    } else {
        afterState = processor_.parameters.copyState();
        applyState(originalState);
        showingBefore = true;
        statusLabel.setText(pas1::ui::utf8String("Показан звук до генерации."), juce::dontSendNotification);
    }
}

void GeneratorPanel::keepCurrent() {
    originalState = processor_.parameters.copyState();
    afterState = originalState.createCopy();
    previewTransaction = false;
    showingBefore = false;
    undoStates.clear();
    redoStates.clear();
    statusLabel.setText(pas1::ui::utf8String("Текущий звук сохранён как новая исходная точка."),
                        juce::dontSendNotification);
}

void GeneratorPanel::revert() {
    if (!previewTransaction || !originalState.isValid())
        return;
    applyState(originalState);
    afterState = originalState.createCopy();
    previewTransaction = false;
    showingBefore = false;
    undoStates.clear();
    redoStates.clear();
    statusLabel.setText(pas1::ui::utf8String("Предыдущий звук восстановлен."),
                        juce::dontSendNotification);
}

void GeneratorPanel::saveRecipe() {
    processor_.presetManager.getUserPresetDirectory().createDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        pas1::ui::utf8String("Сохранить звуковой рецепт"),
        processor_.presetManager.getUserPresetDirectory().getChildFile(
            pas1::ui::utf8String("Звуковой рецепт.pas1recipe")),
        "*.pas1recipe");
    juce::Component::SafePointer<GeneratorPanel> safeThis(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [safeThis](const juce::FileChooser& chooser) {
        if (safeThis == nullptr)
            return;
        const auto destination = chooser.getResult();
        if (destination == juce::File{})
            return;

        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty("format", "PAS1SoundRecipe");
        object->setProperty("schemaVersion", 1);
        object->setProperty("description", safeThis->recipeText.getText());
        const auto selectedMode = std::clamp(safeThis->variationMode.getSelectedItemIndex(), 0, 9);
        object->setProperty("variationMode", serializedVariationModes[static_cast<std::size_t>(selectedMode)]);
        object->setProperty("variationAmount", safeThis->variationAmount.getValue());
        object->setProperty("seed", static_cast<juce::int64>(safeThis->getSeed()));
        const auto controls = safeThis->getRecipeControls();
        auto controlObject = std::make_unique<juce::DynamicObject>();
        controlObject->setProperty("character", controls.character);
        controlObject->setProperty("brightness", controls.brightness);
        controlObject->setProperty("density", controls.density);
        controlObject->setProperty("movement", controls.movement);
        controlObject->setProperty("aggression", controls.aggression);
        controlObject->setProperty("space", controls.space);
        controlObject->setProperty("harmonicComplexity", controls.harmonicComplexity);
        controlObject->setProperty("bassWeight", controls.bassWeight);
        controlObject->setProperty("stereoWidth", controls.stereoWidth);
        object->setProperty("controls", juce::var(controlObject.release()));
        object->setProperty("lockedParameters", safeThis->lockedParameters.getText());
        auto sectionLocks = std::make_unique<juce::DynamicObject>();
        sectionLocks->setProperty("oscillators",
                                  safeThis->sectionLockButtons[0].getToggleState());
        sectionLocks->setProperty("filterAndEnvelope",
                                  safeThis->sectionLockButtons[1].getToggleState());
        sectionLocks->setProperty("modulation",
                                  safeThis->sectionLockButtons[2].getToggleState());
        sectionLocks->setProperty("effects",
                                  safeThis->sectionLockButtons[3].getToggleState());
        object->setProperty("lockedSections", juce::var(sectionLocks.release()));
        if (!destination.replaceWithText(juce::JSON::toString(juce::var(object.release()), true)))
            safeThis->showMessage(pas1::ui::utf8String("Не удалось записать файл рецепта."));
    });
}

void GeneratorPanel::loadRecipe() {
    processor_.presetManager.getUserPresetDirectory().createDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        pas1::ui::utf8String("Загрузить звуковой рецепт PAS 1"),
        processor_.presetManager.getUserPresetDirectory().getChildFile(
            pas1::ui::utf8String("Звуковой рецепт.pas1recipe")),
        "*.pas1recipe");
    juce::Component::SafePointer<GeneratorPanel> safeThis(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
                             [safeThis](const juce::FileChooser& chooser) {
        if (safeThis == nullptr)
            return;
        const auto source = chooser.getResult();
        if (!source.existsAsFile())
            return;
        if (source.getSize() > 1024 * 1024) {
            safeThis->showMessage(pas1::ui::utf8String("Размер файла рецепта превышает 1 МБ."));
            return;
        }

        const auto document = juce::JSON::parse(source.loadFileAsString());
        if (!document.isObject()
            || document.getProperty("format", {}).toString() != "PAS1SoundRecipe"
            || static_cast<int>(document.getProperty("schemaVersion", 0)) != 1) {
            safeThis->showMessage(pas1::ui::utf8String("Формат файла рецепта PAS 1 не поддерживается."));
            return;
        }

        const auto modeName = document.getProperty("variationMode", {}).toString();
        auto modeIndex = -1;
        for (int index = 0; index < safeThis->variationMode.getNumItems(); ++index) {
            if (safeThis->variationMode.getItemText(index).equalsIgnoreCase(modeName)
                || juce::String(serializedVariationModes[static_cast<std::size_t>(index)])
                       .equalsIgnoreCase(modeName)) {
                modeIndex = index;
                break;
            }
        }
        const auto controls = document.getProperty("controls", {});
        if (modeIndex < 0 || !controls.isObject()) {
            safeThis->showMessage(pas1::ui::utf8String("В рецепте отсутствует корректный режим вариации или набор настроек."));
            return;
        }

        constexpr std::array<const char*, 9> controlKeys{
            "character", "brightness", "density", "movement", "aggression", "space",
            "harmonicComplexity", "bassWeight", "stereoWidth"};
        std::array<double, controlKeys.size()> controlValues{};
        for (std::size_t index = 0; index < controlKeys.size(); ++index) {
            controlValues[index] = std::clamp(
                static_cast<double>(controls.getProperty(controlKeys[index], 0.5)),
                0.0, 1.0);
        }

        constexpr std::array<const char*, 4> sectionKeys{
            "oscillators", "filterAndEnvelope", "modulation", "effects"};
        const auto sectionLocks = document.getProperty("lockedSections", {});
        for (std::size_t index = 0; index < controlValues.size(); ++index)
            safeThis->controls[index].setValue(controlValues[index], juce::dontSendNotification);

        safeThis->recipeText.setText(
            document.getProperty("description", {}).toString(), juce::dontSendNotification);
        safeThis->variationMode.setSelectedItemIndex(modeIndex, juce::dontSendNotification);
        safeThis->variationAmount.setValue(
            std::clamp(static_cast<double>(document.getProperty("variationAmount", 35.0)),
                       0.0, 100.0),
            juce::dontSendNotification);
        const auto seed = std::clamp<juce::int64>(
            static_cast<juce::int64>(document.getProperty("seed", 1)),
            0, std::numeric_limits<std::uint32_t>::max());
        safeThis->seedText.setText(juce::String(static_cast<std::uint32_t>(seed)),
                                   juce::dontSendNotification);
        safeThis->lockedParameters.setText(
            document.getProperty("lockedParameters", "master, polyphony").toString(),
            juce::dontSendNotification);
        for (std::size_t index = 0; index < sectionKeys.size(); ++index) {
            const auto locked = sectionLocks.isObject()
                                    ? static_cast<bool>(sectionLocks.getProperty(
                                          sectionKeys[index], false))
                                    : false;
            safeThis->sectionLockButtons[index].setToggleState(
                locked, juce::dontSendNotification);
        }

        safeThis->variations.clear();
        safeThis->variationSeeds.clear();
        safeThis->variationSelector.clear(juce::dontSendNotification);
        safeThis->statusLabel.setText(
            pas1::ui::utf8String("Рецепт загружен и готов к прослушиванию. Текущий звук не изменён."),
            juce::dontSendNotification);
    });
}

void GeneratorPanel::showMessage(const juce::String& message) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, pas1::ui::utf8String("Генератор PAS 1"),
        message, pas1::ui::utf8String("ОК"), this);
}

std::vector<std::string> GeneratorPanel::getLockedParameters() const {
    juce::StringArray tokens;
    tokens.addTokens(lockedParameters.getText(), ",", "\"");
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(tokens.size()));
    for (auto token : tokens) {
        token = token.trim();
        if (token.isNotEmpty())
            result.push_back(token.toStdString());
    }
    constexpr std::array<pas1::generation::VariationSection, 4> sections{
        pas1::generation::VariationSection::oscillators,
        pas1::generation::VariationSection::filterAndEnvelope,
        pas1::generation::VariationSection::modulation,
        pas1::generation::VariationSection::effects};
    const auto parameters = captureParameters();
    for (std::size_t sectionIndex = 0; sectionIndex < sectionLockButtons.size(); ++sectionIndex) {
        if (!sectionLockButtons[sectionIndex].getToggleState())
            continue;
        for (const auto& parameter : parameters) {
            if (!pas1::generation::SoundGenerator::belongsToSection(
                    parameter.id, sections[sectionIndex]))
                continue;
            if (std::find(result.begin(), result.end(), parameter.id) == result.end())
                result.push_back(parameter.id);
        }
    }
    return result;
}

pas1::generation::RecipeControls GeneratorPanel::getRecipeControls() const {
    return {static_cast<float>(controls[0].getValue()),
            static_cast<float>(controls[1].getValue()),
            static_cast<float>(controls[2].getValue()),
            static_cast<float>(controls[3].getValue()),
            static_cast<float>(controls[4].getValue()),
            static_cast<float>(controls[5].getValue()),
            static_cast<float>(controls[6].getValue()),
            static_cast<float>(controls[7].getValue()),
            static_cast<float>(controls[8].getValue())};
}

pas1::generation::VariationMode GeneratorPanel::getVariationMode() const {
    const auto selected = std::clamp(variationMode.getSelectedItemIndex(), 0, 9);
    return static_cast<pas1::generation::VariationMode>(selected);
}

std::uint32_t GeneratorPanel::getSeed() const {
    const auto parsed = seedText.getText().getLargeIntValue();
    return static_cast<std::uint32_t>(
        std::clamp<juce::int64>(parsed, 0, std::numeric_limits<std::uint32_t>::max()));
}
