#include "GeneratorPanel.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {
const juce::Colour panelBackground{0xff302015};
const juce::Colour panelAccent{0xffd8b36a};
const juce::Colour panelText{0xfff4e8cf};
const juce::Colour panelMuted{0xffbba483};

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
        "Describe a sound, e.g. warm evolving analog bass with metallic attack and a wide tail.",
        panelMuted);
    recipeText.setColour(juce::TextEditor::backgroundColourId, panelBackground);
    recipeText.setColour(juce::TextEditor::textColourId, panelText);
    recipeText.setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff745332});
    addAndMakeVisible(recipeText);

    variationMode.addItemList(
        {"Full Random", "Subtle", "Aggressive", "Genre", "Morph", "Texture",
         "Preset Mutation", "Generate Similar", "Surprise Me", "Seeded"}, 1);
    variationMode.setSelectedId(2, juce::dontSendNotification);
    variationMode.setColour(juce::ComboBox::backgroundColourId, panelBackground);
    variationMode.setColour(juce::ComboBox::textColourId, panelText);
    variationMode.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff745332});
    addAndMakeVisible(variationMode);

    variationAmount.setSliderStyle(juce::Slider::LinearHorizontal);
    variationAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 22);
    variationAmount.setRange(0.0, 100.0, 1.0);
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
    addAndMakeVisible(seedText);
    lockedParameters.setText("master, polyphony", juce::dontSendNotification);
    lockedParameters.setColour(juce::TextEditor::backgroundColourId, panelBackground);
    lockedParameters.setColour(juce::TextEditor::textColourId, panelText);
    lockedParameters.setColour(juce::TextEditor::outlineColourId, juce::Colour{0xff745332});
    addAndMakeVisible(lockedParameters);

    variationSelector.setTextWhenNothingSelected("Choose a variation");
    variationSelector.setColour(juce::ComboBox::backgroundColourId, panelBackground);
    variationSelector.setColour(juce::ComboBox::textColourId, panelText);
    variationSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff745332});
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
                         &keepButton, &revertButton, &applyButton, &saveRecipeButton})
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
        statusLabel.setText("Applied. Continue editing or save it as a preset.", juce::dontSendNotification);
    };
    saveRecipeButton.onClick = [this] { saveRecipe(); };

    for (auto* pair : {&amountLabel, &modeLabel, &seedLabel, &lockedLabel, &variationLabel})
        pair->setColour(juce::Label::textColourId, panelMuted);
    amountLabel.setText("Variation Amount", juce::dontSendNotification);
    modeLabel.setText("Mode", juce::dontSendNotification);
    seedLabel.setText("Seed", juce::dontSendNotification);
    lockedLabel.setText("Locked parameter IDs", juce::dontSendNotification);
    variationLabel.setText("Generated set", juce::dontSendNotification);
    for (auto* label : {&amountLabel, &modeLabel, &seedLabel, &lockedLabel, &variationLabel})
        addAndMakeVisible(*label);

    statusLabel.setColour(juce::Label::textColourId, panelMuted);
    statusLabel.setText("Local rule-based generator — no AI model or network connection.", juce::dontSendNotification);
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
    lockedLabel.setBounds(654, controlY, width - 666, 18);
    lockedParameters.setBounds(654, controlY + 16, width - 666, 28);

    const auto buttonY = 414;
    const auto buttonGap = 8;
    const auto buttonWidth = std::max(62, (width - 40 - 6 * buttonGap) / 7);
    juce::TextButton* firstRow[]{
        &recipeButton, &variationButton, &generateFourButton, &generateSixteenButton,
        &beforeAfterButton, &undoButton, &redoButton};
    for (std::size_t index = 0; index < std::size(firstRow); ++index)
        firstRow[index]->setBounds(12 + static_cast<int>(index) * (buttonWidth + buttonGap),
                                   buttonY, buttonWidth, 30);

    juce::TextButton* secondRow[]{&keepButton, &revertButton, &applyButton, &saveRecipeButton};
    const auto secondWidth = std::max(100, (width - 48) / 4);
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
        captureParameters(), recipeText.getText().toStdString(), getRecipeControls());
    applyParameters(result);
    statusLabel.setText("Recipe preview active. Use Apply to keep it or Revert to restore the previous sound.",
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
    statusLabel.setText("Variation preview active. The selected seed can reproduce this result.",
                        juce::dontSendNotification);
}

void GeneratorPanel::generateVariations(int count) {
    beginPreview();
    variations.clear();
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

    for (int index = 0; index < count; ++index) {
        const auto result = pas1::generation::SoundGenerator::createVariation(
            base, getVariationMode(), static_cast<float>(variationAmount.getValue() / 100.0),
            seed + static_cast<std::uint32_t>(index) * 2654435761u,
            getLockedParameters(), recipeText.getText().toStdString());
        variations.push_back(makeSnapshot(baseState, result));
        variationSelector.addItem("Variation " + juce::String(index + 1), index + 1);
    }

    if (!variations.empty()) {
        variationSelector.setSelectedId(1, juce::sendNotification);
        statusLabel.setText(juce::String(count) + " variations generated. Select one to preview.",
                            juce::dontSendNotification);
    }
}

void GeneratorPanel::selectVariation(int index) {
    if (index < 0 || index >= static_cast<int>(variations.size()))
        return;
    pushUndoState();
    applyState(variations[static_cast<std::size_t>(index)]);
    afterState = processor_.parameters.copyState();
    statusLabel.setText("Previewing variation " + juce::String(index + 1)
                            + ". Apply to keep it or Revert to restore the previous sound.",
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
        statusLabel.setText("Showing generated sound.", juce::dontSendNotification);
    } else {
        afterState = processor_.parameters.copyState();
        applyState(originalState);
        showingBefore = true;
        statusLabel.setText("Showing sound before generation.", juce::dontSendNotification);
    }
}

void GeneratorPanel::keepCurrent() {
    originalState = processor_.parameters.copyState();
    afterState = originalState.createCopy();
    previewTransaction = false;
    showingBefore = false;
    undoStates.clear();
    redoStates.clear();
    statusLabel.setText("Current sound kept as the new starting point.", juce::dontSendNotification);
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
    statusLabel.setText("Previous sound restored.", juce::dontSendNotification);
}

void GeneratorPanel::saveRecipe() {
    processor_.presetManager.getUserPresetDirectory().createDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Sound Recipe",
        processor_.presetManager.getUserPresetDirectory().getChildFile("Sound Recipe.pas1recipe"),
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
        object->setProperty("variationMode", safeThis->variationMode.getText());
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
        if (!destination.replaceWithText(juce::JSON::toString(juce::var(object.release()), true)))
            safeThis->showMessage("Could not write the recipe file.");
    });
}

void GeneratorPanel::showMessage(const juce::String& message) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "PAS 1 Generator", message, "OK", this);
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
    const auto parsed = static_cast<std::uint32_t>(seedText.getText().getIntValue());
    return parsed;
}
