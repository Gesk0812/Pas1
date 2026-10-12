#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Generation/SoundGenerator.h"
#include "PluginProcessor.h"

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
        juce::TextButton{"OSC"}, juce::TextButton{"FILTER"}, juce::TextButton{"MOD"},
        juce::TextButton{"FX"}};
    juce::Label amountLabel, modeLabel, seedLabel, lockedLabel, variationLabel;
    std::array<Slider, 9> controls{};
    std::array<juce::Label, 9> controlLabels{};
    std::array<juce::String, 9> controlNames{
        "Character", "Brightness", "Density", "Movement", "Aggression",
        "Space", "Harmonic", "Bass Weight", "Stereo Width"};
    juce::TextButton recipeButton{"RECIPE PREVIEW"};
    juce::TextButton variationButton{"VARIATION"};
    juce::TextButton generateFourButton{"GENERATE 4"};
    juce::TextButton generateSixteenButton{"GENERATE 16"};
    juce::TextButton beforeAfterButton{"BEFORE / AFTER"};
    juce::TextButton undoButton{"UNDO"};
    juce::TextButton redoButton{"REDO"};
    juce::TextButton keepButton{"KEEP"};
    juce::TextButton revertButton{"REVERT"};
    juce::TextButton applyButton{"APPLY"};
    juce::TextButton saveRecipeButton{"SAVE RECIPE"};
    juce::TextButton loadRecipeButton{"LOAD RECIPE"};
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
