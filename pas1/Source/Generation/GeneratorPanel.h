#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Generation/SoundGenerator.h"
#include "PluginProcessor.h"
#include "UI/Utf8Text.h"

#include <array>
#include <vector>

class GeneratorPanel final : public juce::Component {
public:
    explicit GeneratorPanel(Pas1AudioProcessor& processor);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using Slider = juce::Slider;
    using ParameterList = std::vector<pas1::generation::ParameterValue>;

    ParameterList captureParameters() const;
    juce::ValueTree makeSnapshot(const juce::ValueTree& base,
                                 const ParameterList& values) const;
    void applyParameters(const ParameterList& values);
    void applyState(const juce::ValueTree& state);
    void beginPreview();
    void previewRecipe();
    void previewVariation();
    void generateVariations(int count);
    void selectVariation(int index);
    void pushUndoState();
    void undo();
    void redo();
    void showBeforeAfter();
    void keepCurrent();
    void revert();
    void saveRecipe();
    void loadRecipe();
    void showMessage(const juce::String& message);
    void updateRecipeExplanation();
    std::vector<std::string> getLockedParameters() const;
    pas1::generation::RecipeControls getRecipeControls() const;
    pas1::generation::VariationMode getVariationMode() const;
    std::uint32_t getSeed() const;

    Pas1AudioProcessor& processor_;
    juce::TextEditor recipeText;
    juce::ComboBox variationMode;
    juce::Slider variationAmount;
    juce::TextEditor seedText;
    juce::TextEditor lockedParameters;
    juce::ComboBox variationSelector;
    std::array<juce::TextButton, 4> sectionLockButtons{
        juce::TextButton{pas1::ui::utf8String("ОСЦ.")},
        juce::TextButton{pas1::ui::utf8String("ФИЛЬТР")},
        juce::TextButton{pas1::ui::utf8String("МОД.")},
        juce::TextButton{pas1::ui::utf8String("ЭФФ.")}};
    juce::Label amountLabel, modeLabel, seedLabel, lockedLabel, variationLabel;
    juce::Label interpretationLabel;
    std::array<Slider, 9> controls{};
    std::array<juce::Label, 9> controlLabels{};
    std::array<juce::String, 9> controlNames{
        pas1::ui::utf8String("Характер"), pas1::ui::utf8String("Яркость"),
        pas1::ui::utf8String("Плотность"), pas1::ui::utf8String("Движение"),
        pas1::ui::utf8String("Агрессивность"), pas1::ui::utf8String("Пространство"),
        pas1::ui::utf8String("Гармоники"), pas1::ui::utf8String("Вес баса"),
        pas1::ui::utf8String("Ширина стерео")};
    juce::TextButton recipeButton{pas1::ui::utf8String("РЕЦЕПТ")};
    juce::TextButton variationButton{pas1::ui::utf8String("ВАРИАЦИЯ")};
    juce::TextButton generateFourButton{pas1::ui::utf8String("СОЗДАТЬ 4")};
    juce::TextButton generateSixteenButton{pas1::ui::utf8String("СОЗДАТЬ 16")};
    juce::TextButton beforeAfterButton{pas1::ui::utf8String("ДО / ПОСЛЕ")};
    juce::TextButton undoButton{pas1::ui::utf8String("ОТМЕНИТЬ")};
    juce::TextButton redoButton{pas1::ui::utf8String("ПОВТОРИТЬ")};
    juce::TextButton keepButton{pas1::ui::utf8String("ОСТАВИТЬ")};
    juce::TextButton revertButton{pas1::ui::utf8String("ВЕРНУТЬ")};
    juce::TextButton applyButton{pas1::ui::utf8String("ПРИМЕНИТЬ")};
    juce::TextButton saveRecipeButton{pas1::ui::utf8String("СОХРАНИТЬ РЕЦЕПТ")};
    juce::TextButton loadRecipeButton{pas1::ui::utf8String("ЗАГРУЗИТЬ РЕЦЕПТ")};
    juce::Label statusLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::ValueTree originalState;
    juce::ValueTree afterState;
    std::vector<juce::ValueTree> variations;
    std::vector<std::uint32_t> variationSeeds;
    std::vector<juce::ValueTree> undoStates;
    std::vector<juce::ValueTree> redoStates;
    bool previewTransaction = false;
    bool showingBefore = false;
};
