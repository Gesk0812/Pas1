#include "ModulationPanel.h"

namespace {
const juce::Colour rowBackground{0xff22242a};
const juce::Colour fieldBackground{0xff181a1f};
const juce::Colour accent{0xffcf7bff};
const juce::Colour primaryText{0xfff4f0f7};
const juce::Colour secondaryText{0xffaaa4b1};

void styleCombo(juce::ComboBox& combo) {
    combo.setColour(juce::ComboBox::backgroundColourId, fieldBackground);
    combo.setColour(juce::ComboBox::textColourId, primaryText);
    combo.setColour(juce::ComboBox::outlineColourId, juce::Colour{0xff393641});
    combo.setColour(juce::ComboBox::arrowColourId, accent);
}
} // namespace

ModulationPanel::RouteRow::RouteRow(juce::AudioProcessorValueTreeState& state,
                                    std::size_t routeIndex)
    : routeIndex_(routeIndex) {
    juce::StringArray sourceNames{
        "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Amp Envelope", "Filter Envelope",
        "Velocity", "Mod Wheel", "Pitch Bend", "Key Tracking", "Random per Note"};
    juce::StringArray destinationNames{
        "Pitch", "Osc 1 Level", "Wavetable Position", "Filter Cutoff", "Resonance",
        "FM Amount", "Pan", "Distortion Mix", "Reverb Mix", "Morph X", "Morph Y"};
    source.addItemList(sourceNames, 1);
    destination.addItemList(destinationNames, 1);
    polarity.addItemList({"Bipolar", "Unipolar"}, 1);
    for (auto* combo : {&source, &destination, &polarity}) {
        styleCombo(*combo);
        addAndMakeVisible(*combo);
    }

    amount.setSliderStyle(juce::Slider::LinearHorizontal);
    amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 22);
    amount.setRange(-1.0, 1.0, 0.001);
    amount.setColour(juce::Slider::trackColourId, accent);
    amount.setColour(juce::Slider::thumbColourId, primaryText);
    amount.setColour(juce::Slider::textBoxTextColourId, primaryText);
    amount.setColour(juce::Slider::textBoxBackgroundColourId, fieldBackground);
    amount.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour{0xff393641});
    amount.setTooltip("Modulation depth. Negative values invert the modulation.");
    addAndMakeVisible(amount);

    enabled.setClickingTogglesState(true);
    enabled.setColour(juce::ToggleButton::textColourId, secondaryText);
    enabled.setColour(juce::ToggleButton::tickColourId, accent);
    enabled.setTooltip("Enable or bypass this modulation route.");
    addAndMakeVisible(enabled);

    const auto prefix = "modroute" + std::to_string(routeIndex_ + 1);
    sourceAttachment = std::make_unique<ComboBoxAttachment>(
        state, prefix + "source", source);
    destinationAttachment = std::make_unique<ComboBoxAttachment>(
        state, prefix + "destination", destination);
    amountAttachment = std::make_unique<SliderAttachment>(
        state, prefix + "amount", amount);
    polarityAttachment = std::make_unique<ComboBoxAttachment>(
        state, prefix + "polarity", polarity);
    enabledAttachment = std::make_unique<ButtonAttachment>(
        state, prefix + "enabled", enabled);
}

void ModulationPanel::RouteRow::paint(juce::Graphics& graphics) {
    graphics.setColour(rowBackground.withAlpha(routeIndex_ % 2 == 0 ? 0.92f : 0.58f));
    graphics.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f);
    graphics.setColour(secondaryText);
    graphics.setFont(juce::FontOptions{10.0f, juce::Font::bold});
    graphics.drawText(juce::String(routeIndex_ + 1).paddedLeft('0', 2),
                      8, 0, 26, getHeight(), juce::Justification::centredLeft);
}

void ModulationPanel::RouteRow::resized() {
    auto x = 38;
    const auto y = 5;
    const auto height = getHeight() - 10;
    source.setBounds(x, y, 138, height);
    x += 146;
    destination.setBounds(x, y, 165, height);
    x += 173;
    const auto fixedAfterAmount = 8 + 110 + 8 + 54 + 8;
    const auto amountWidth = juce::jmax(150, getWidth() - x - fixedAfterAmount);
    amount.setBounds(x, y, amountWidth, height);
    x += amountWidth + 8;
    polarity.setBounds(x, y, 110, height);
    x += 118;
    enabled.setBounds(x, y, 54, height);
}

ModulationPanel::ModulationPanel(juce::AudioProcessorValueTreeState& state) {
    setOpaque(false);
    viewport.setViewedComponent(&rowsContent, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(10);
    viewport.setOpaque(false);
    addAndMakeVisible(viewport);

    constexpr std::array<const char*, 6> headings{
        "ROUTE", "SOURCE", "DESTINATION", "AMOUNT", "POLARITY", "ACTIVE"};
    for (std::size_t index = 0; index < headings.size(); ++index) {
        auto& label = columnLabels[index];
        label.setText(headings[index], juce::dontSendNotification);
        label.setFont(juce::FontOptions{9.0f, juce::Font::bold});
        label.setColour(juce::Label::textColourId, secondaryText);
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
    }
    for (std::size_t index = 0; index < rows.size(); ++index) {
        rows[index] = std::make_unique<RouteRow>(state, index);
        rowsContent.addAndMakeVisible(*rows[index]);
    }
}

void ModulationPanel::resized() {
    const auto width = getWidth();
    const auto contentHeight = 38 * static_cast<int>(rows.size());
    rowsContent.setSize(width, contentHeight);
    for (std::size_t index = 0; index < rows.size(); ++index)
        rows[index]->setBounds(0, static_cast<int>(index) * 38, width, 36);

    auto x = 8;
    columnLabels[0].setBounds(x, 0, 34, 24);
    x += 38;
    columnLabels[1].setBounds(x, 0, 138, 24);
    x += 146;
    columnLabels[2].setBounds(x, 0, 165, 24);
    x += 173;
    const auto fixedAfterAmount = 8 + 110 + 8 + 54 + 8;
    const auto amountWidth = juce::jmax(150, width - x - fixedAfterAmount);
    columnLabels[3].setBounds(x, 0, amountWidth, 24);
    x += amountWidth + 8;
    columnLabels[4].setBounds(x, 0, 110, 24);
    x += 118;
    columnLabels[5].setBounds(x, 0, 54, 24);
    viewport.setBounds(0, 26, width, juce::jmax(0, getHeight() - 26));
}
