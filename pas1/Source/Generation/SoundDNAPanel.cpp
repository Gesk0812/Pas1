#include "SoundDNAPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {
const juce::Colour background{0xff302015};
const juce::Colour accent{0xffd8b36a};
const juce::Colour text{0xfff4e8cf};
const juce::Colour muted{0xffbba483};
const juce::Colour outline{0xff745332};

const std::array<juce::String, pas1::dsp::SoundDNAProfile::featureCount> featureNames{
    pas1::ui::utf8String("Яркость"), pas1::ui::utf8String("Гармоническая сложность"),
    pas1::ui::utf8String("Плотность"), pas1::ui::utf8String("Движение"),
    pas1::ui::utf8String("Агрессивность"), pas1::ui::utf8String("Ширина стерео"),
    pas1::ui::utf8String("Вес баса"), pas1::ui::utf8String("Пространство"),
    pas1::ui::utf8String("Текстура"), pas1::ui::utf8String("Характер огибающей")};

void styleButton(juce::TextButton& button) {
    button.setColour(juce::TextButton::buttonColourId, background);
    button.setColour(juce::TextButton::textColourOffId, text);
    button.setColour(juce::TextButton::textColourOnId, accent);
}

void styleCombo(juce::ComboBox& combo) {
    combo.setColour(juce::ComboBox::backgroundColourId, background);
    combo.setColour(juce::ComboBox::textColourId, text);
    combo.setColour(juce::ComboBox::outlineColourId, outline);
}
}

SoundDNAPanel::SoundDNAPanel(Pas1AudioProcessor& processor) : processor_(processor) {
    for (int index = 0; index < PresetManager::getBuiltInPresetCount(); ++index) {
        const auto info = PresetManager::getBuiltInPresetInfo(index);
        presetA.addItem(info.name, index + 1);
        presetB.addItem(info.name, index + 1);
    }
    presetA.setSelectedId(1, juce::dontSendNotification);
    presetB.setSelectedId(5, juce::dontSendNotification);
    similarMatches.setTextWhenNothingSelected(pas1::ui::utf8String("Сначала найдите похожие пресеты"));
    styleCombo(presetA);
    styleCombo(presetB);
    styleCombo(similarMatches);
    addAndMakeVisible(presetA);
    addAndMakeVisible(presetB);
    addAndMakeVisible(similarMatches);

    blendAmount.setSliderStyle(juce::Slider::LinearHorizontal);
    blendAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 22);
    blendAmount.setRange(0.0, 100.0, 1.0);
    blendAmount.setValue(50.0, juce::dontSendNotification);
    blendAmount.setTextValueSuffix("%");
    blendAmount.setColour(juce::Slider::trackColourId, accent);
    blendAmount.setColour(juce::Slider::thumbColourId, text);
    blendAmount.setColour(juce::Slider::textBoxTextColourId, text);
    blendAmount.setColour(juce::Slider::textBoxBackgroundColourId, background);
    addAndMakeVisible(blendAmount);

    for (auto* label : {&presetALabel, &presetBLabel, &blendLabel, &status})
        label->setColour(juce::Label::textColourId, muted);
    presetALabel.setText(pas1::ui::utf8String("ПРЕСЕТ A"), juce::dontSendNotification);
    presetBLabel.setText(pas1::ui::utf8String("ПРЕСЕТ B"), juce::dontSendNotification);
    blendLabel.setText(pas1::ui::utf8String("СМЕШИВАНИЕ ДНК"), juce::dontSendNotification);
    for (auto* label : {&presetALabel, &presetBLabel, &blendLabel, &status})
        addAndMakeVisible(*label);

    for (auto* button : {&analyzeButton, &similarButton, &compareButton, &blendButton,
                         &exploreButton, &generateButton, &loadMatchButton, &keepButton,
                         &revertButton}) {
        styleButton(*button);
        addAndMakeVisible(*button);
    }
    analyzeButton.onClick = [this] { analyzeCurrent(); };
    similarButton.onClick = [this] { findSimilar(); };
    compareButton.onClick = [this] { compareToBuiltIn(); };
    blendButton.onClick = [this] { blendSelected(); };
    exploreButton.onClick = [this] { exploreSoundSpace(); };
    generateButton.onClick = [this] { generateFromDNA(); };
    loadMatchButton.onClick = [this] { loadSelectedMatch(); };
    keepButton.onClick = [this] { keepCurrent(); };
    revertButton.onClick = [this] { revert(); };

    analyzeCurrent();
}

SoundDNAPanel::ParameterList SoundDNAPanel::captureParameters(bool useDefaults) const {
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
        const auto value = useDefaults ? parameter->getDefaultValue() : parameter->getValue();
        result.push_back({id.toStdString(), value, parameter->getDefaultValue(), range.start,
                          range.end, range.skew, parameter->getNumSteps(), discrete});
    }
    return result;
}

SoundDNAPanel::ParameterList SoundDNAPanel::builtInParameters(int index) const {
    auto result = captureParameters(true);
    for (const auto& setting : PresetManager::getBuiltInPresetValues(index)) {
        const auto* parameter = processor_.parameters.getParameter(setting.first);
        if (parameter == nullptr)
            continue;
        const auto normalized = parameter->convertTo0to1(setting.second);
        const auto found = std::find_if(result.begin(), result.end(),
                                        [&setting](const auto& value) {
                                            return value.id == setting.first.toStdString();
                                        });
        if (found != result.end())
            found->normalizedValue = normalized;
    }
    return result;
}

pas1::dsp::SoundDNAProfile SoundDNAPanel::builtInProfile(int index) const {
    return pas1::dsp::SoundDNA::analyze(builtInParameters(index));
}

void SoundDNAPanel::analyzeCurrent() {
    currentProfile = pas1::dsp::SoundDNA::analyze(captureParameters());
    pendingProfile.reset();
    displayProfile = currentProfile;
    setStatus(pas1::ui::utf8String("Текущий пресет проанализирован по параметрам синтезатора."));
    repaint();
}

void SoundDNAPanel::findSimilar() {
    const auto profile = currentProfile;
    std::vector<std::pair<float, int>> ranked;
    ranked.reserve(static_cast<std::size_t>(PresetManager::getBuiltInPresetCount()));
    for (int index = 0; index < PresetManager::getBuiltInPresetCount(); ++index)
        ranked.emplace_back(pas1::dsp::SoundDNA::distance(profile, builtInProfile(index)), index);
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& left, const auto& right) { return left.first < right.first; });

    similarPresetIndices.clear();
    similarMatches.clear(juce::dontSendNotification);
    juce::String results;
    const auto resultCount = std::min<std::size_t>(3, ranked.size());
    for (std::size_t index = 0; index < resultCount; ++index) {
        const auto presetIndex = ranked[index].second;
        const auto info = PresetManager::getBuiltInPresetInfo(presetIndex);
        similarPresetIndices.push_back(presetIndex);
        similarMatches.addItem(info.name + " · " + juce::String(ranked[index].first, 2),
                               static_cast<int>(index) + 1);
        if (results.isNotEmpty())
            results += ", ";
        results += info.name;
    }
    if (!similarPresetIndices.empty()) {
        similarMatches.setSelectedId(1, juce::dontSendNotification);
        setStatus(pas1::ui::utf8String("Наиболее похожие заводские пресеты: ")
                  + results + pas1::ui::utf8String("."));
    } else {
        setStatus(pas1::ui::utf8String("Похожих заводских пресетов не найдено."));
    }
    repaint();
}

void SoundDNAPanel::compareToBuiltIn() {
    const auto index = presetA.getSelectedId() - 1;
    if (index < 0 || index >= PresetManager::getBuiltInPresetCount())
        return;
    currentProfile = pas1::dsp::SoundDNA::analyze(captureParameters());
    const auto other = builtInProfile(index);
    pendingProfile.reset();
    const auto difference = pas1::dsp::SoundDNA::distance(currentProfile, other);
    displayProfile = currentProfile;
    setStatus(pas1::ui::utf8String("Расстояние до пресета «")
              + PresetManager::getBuiltInPresetInfo(index).name
              + pas1::ui::utf8String("»: ") + juce::String(difference, 3)
              + pas1::ui::utf8String(" (0 — совпадение, 1 — максимальное различие)."));
    repaint();
}

void SoundDNAPanel::blendSelected() {
    const auto firstIndex = presetA.getSelectedId() - 1;
    const auto secondIndex = presetB.getSelectedId() - 1;
    if (firstIndex < 0 || secondIndex < 0)
        return;
    const auto first = builtInProfile(firstIndex);
    const auto second = builtInProfile(secondIndex);
    pendingProfile = pas1::dsp::SoundDNA::blend(first, second, getBlendAmount());
    displayProfile = *pendingProfile;
    setStatus(pas1::ui::utf8String("Смешаны профили пресетов «")
              + PresetManager::getBuiltInPresetInfo(firstIndex).name
              + pas1::ui::utf8String("» и «")
              + PresetManager::getBuiltInPresetInfo(secondIndex).name
              + pas1::ui::utf8String("»."));
    repaint();
}

void SoundDNAPanel::exploreSoundSpace() {
    currentProfile = pas1::dsp::SoundDNA::analyze(captureParameters());
    int farthestIndex = 0;
    auto farthestDistance = -1.0f;
    for (int index = 0; index < PresetManager::getBuiltInPresetCount(); ++index) {
        const auto distance = pas1::dsp::SoundDNA::distance(currentProfile, builtInProfile(index));
        if (distance > farthestDistance) {
            farthestDistance = distance;
            farthestIndex = index;
        }
    }
    pendingProfile = pas1::dsp::SoundDNA::blend(
        currentProfile, builtInProfile(farthestIndex), getBlendAmount());
    displayProfile = *pendingProfile;
    setStatus("Создан промежуточный профиль между текущим звуком и пресетом «"
              + PresetManager::getBuiltInPresetInfo(farthestIndex).name + "».");
    repaint();
}

void SoundDNAPanel::generateFromDNA() {
    const auto profile = pendingProfile.value_or(
        pas1::dsp::SoundDNA::analyze(captureParameters()));
    if (!previewOpen) {
        originalState = processor_.parameters.copyState();
        previewOpen = true;
    }
    const auto generated = pas1::generation::SoundGenerator::createRecipe(
        captureParameters(), std::string{},
        pas1::dsp::SoundDNA::toRecipeControls(profile));
    for (const auto& value : generated)
        if (auto* parameter = processor_.parameters.getParameter(juce::String(value.id)))
            parameter->setValueNotifyingHost(value.normalizedValue);
    currentProfile = pas1::dsp::SoundDNA::analyze(captureParameters());
    displayProfile = currentProfile;
    pendingProfile.reset();
    setStatus("Звук создан по ДНК. Оставьте его или верните прежнее состояние.");
    repaint();
}

void SoundDNAPanel::keepCurrent() {
    previewOpen = false;
    originalState = processor_.parameters.copyState();
    pendingProfile.reset();
    setStatus("Текущий звук сохранён как новый исходный.");
}

void SoundDNAPanel::revert() {
    if (!previewOpen || !originalState.isValid())
        return;
    processor_.parameters.replaceState(originalState.createCopy());
    previewOpen = false;
    analyzeCurrent();
    setStatus("Предыдущий звук восстановлен.");
}

void SoundDNAPanel::loadSelectedMatch() {
    const auto selected = similarMatches.getSelectedId() - 1;
    if (selected < 0 || selected >= static_cast<int>(similarPresetIndices.size()))
        return;
    if (processor_.presetManager.loadBuiltInPreset(
            similarPresetIndices[static_cast<std::size_t>(selected)])) {
        analyzeCurrent();
        setStatus("Загружен выбранный похожий заводской пресет.");
    }
}

void SoundDNAPanel::setStatus(const juce::String& message) {
    status.setText(message, juce::dontSendNotification);
}

float SoundDNAPanel::getBlendAmount() const {
    return static_cast<float>(blendAmount.getValue() / 100.0);
}

void SoundDNAPanel::drawProfile(juce::Graphics& graphics,
                                const juce::Rectangle<int>& area) const {
    constexpr int columns = 2;
    constexpr int rows = 5;
    const auto cellWidth = (area.getWidth() - 24) / columns;
    const auto cellHeight = (area.getHeight() - 16) / rows;
    for (std::size_t index = 0; index < displayProfile.values.size(); ++index) {
        const auto column = static_cast<int>(index) / rows;
        const auto row = static_cast<int>(index) % rows;
        const auto x = area.getX() + 8 + column * cellWidth;
        const auto y = area.getY() + 8 + row * cellHeight;
        graphics.setColour(muted);
        graphics.setFont(juce::FontOptions{11.0f});
        graphics.drawText(featureNames[index], x, y, cellWidth - 8, 18,
                          juce::Justification::centredLeft);
        auto track = juce::Rectangle<float>{static_cast<float>(x), static_cast<float>(y + 24),
                                            static_cast<float>(cellWidth - 12), 8.0f};
        graphics.setColour(outline);
        graphics.fillRoundedRectangle(track, 4.0f);
        track.setWidth(track.getWidth() * std::clamp(displayProfile.values[index], 0.0f, 1.0f));
        graphics.setColour(accent);
        graphics.fillRoundedRectangle(track, 4.0f);
    }
}

void SoundDNAPanel::paint(juce::Graphics& graphics) {
    graphics.setColour(background);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
    graphics.setColour(outline);
    graphics.fillRoundedRectangle(10.0f, 12.0f, static_cast<float>(getWidth()) * 0.51f,
                                  static_cast<float>(getHeight() - 24), 8.0f);
    drawProfile(graphics, {12, 14, static_cast<int>(getWidth() * 0.51f) - 4,
                           getHeight() - 28});
}

void SoundDNAPanel::resized() {
    const auto width = getWidth();
    const auto rightX = static_cast<int>(width * 0.54f);
    const auto rightWidth = width - rightX - 16;
    presetALabel.setBounds(rightX, 14, 150, 18);
    presetBLabel.setBounds(rightX + rightWidth / 2 + 4, 14, rightWidth / 2 - 4, 18);
    presetA.setBounds(rightX, 34, rightWidth / 2 - 6, 28);
    presetB.setBounds(rightX + rightWidth / 2 + 4, 34, rightWidth / 2 - 4, 28);

    const auto buttonGap = 6;
    const auto threeWidth = (rightWidth - 2 * buttonGap) / 3;
    analyzeButton.setBounds(rightX, 72, threeWidth, 30);
    similarButton.setBounds(rightX + threeWidth + buttonGap, 72, threeWidth, 30);
    compareButton.setBounds(rightX + 2 * (threeWidth + buttonGap), 72, threeWidth, 30);

    similarMatches.setBounds(rightX, 110, rightWidth - 110, 28);
    loadMatchButton.setBounds(rightX + rightWidth - 104, 110, 104, 28);
    blendLabel.setBounds(rightX, 148, 100, 18);
    blendAmount.setBounds(rightX, 168, rightWidth - 140, 28);
    blendButton.setBounds(rightX + rightWidth - 132, 166, 132, 30);

    const auto halfWidth = (rightWidth - 8) / 2;
    exploreButton.setBounds(rightX, 210, halfWidth, 32);
    generateButton.setBounds(rightX + halfWidth + 8, 210, halfWidth, 32);
    keepButton.setBounds(rightX, 252, halfWidth, 30);
    revertButton.setBounds(rightX + halfWidth + 8, 252, halfWidth, 30);
    status.setBounds(rightX, 302, rightWidth, getHeight() - 316);
}
