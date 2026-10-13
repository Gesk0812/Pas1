#include "ParameterGraphs.h"
#include "Utf8Text.h"

#include "../DSP/Oscillator.h"
#include "../DSP/StateVariableFilter.h"
#include "../DSP/WavetableOscillator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {
constexpr juce::uint32 panelColour = 0xff24160f;
constexpr juce::uint32 gridColour = 0xff745332;
constexpr juce::uint32 accentColour = 0xffd8b36a;
constexpr juce::uint32 textColour = 0xfff4e8cf;
constexpr juce::uint32 mutedColour = 0xffbba483;

float parameterValue(Pas1AudioProcessor& processor, const char* id, float fallback) {
    if (const auto* value = processor.parameters.getRawParameterValue(id))
        return value->load(std::memory_order_relaxed);
    return fallback;
}

juce::String waveformName(int waveform) {
    constexpr std::array<const char*, 12> names{
        "СИНУС", "ТРЕУГОЛЬНИК", "ПИЛА", "ПРЯМОУГОЛЬНИК", "ИМПУЛЬС", "ОБРАТНАЯ ПИЛА",
        "ОРГАН", "ПОЛЫЙ", "ФОРМАНТ", "КОЛОКОЛ", "МЕТАЛЛ", "МЯГКИЙ КЛИППИНГ"};
    return pas1::ui::utf8String(
        names[static_cast<std::size_t>(std::clamp(waveform, 0, 11))]);
}

juce::String filterName(int type) {
    constexpr std::array<const char*, 4> names{"НЧ", "ВЧ", "ПОЛОСОВОЙ", "РЕЖЕКТОРНЫЙ"};
    return pas1::ui::utf8String(names[static_cast<std::size_t>(std::clamp(type, 0, 3))]);
}
} // namespace

void OscillatorWaveformGraph::update() {
    repaint();
}

void OscillatorWaveformGraph::paint(juce::Graphics& graphics) {
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour(juce::Colour{panelColour});
    graphics.fillRoundedRectangle(bounds, 8.0f);

    const auto waveform = std::clamp(
        juce::roundToInt(parameterValue(processor_, "waveform", 0.0f)), 0,
        pas1::dsp::WavetableOscillator::waveformCount - 1);
    const auto scan = std::clamp(parameterValue(processor_, "wavetablescan", 0.0f), 0.0f, 1.0f);
    const auto level = std::clamp(parameterValue(processor_, "osc1level", 1.0f), 0.0f, 1.0f);

    graphics.setColour(juce::Colour{textColour});
    graphics.setFont(juce::FontOptions{10.0f, juce::Font::bold});
    graphics.drawText(waveformName(waveform), 12, 5, 150, 14,
                      juce::Justification::centredLeft);
    graphics.setColour(juce::Colour{mutedColour});
    graphics.setFont(juce::FontOptions{9.0f});
    graphics.drawText("SCAN " + juce::String{scan, 2}
                          + pas1::ui::utf8String("    УРОВЕНЬ ") + juce::String{level * 100.0f, 0} + "%",
                      bounds.getWidth() - 210.0f, 5, 198, 14,
                      juce::Justification::centredRight);

    auto plot = bounds.reduced(12.0f, 22.0f);
    plot.removeFromTop(2.0f);
    graphics.setColour(juce::Colour{gridColour});
    graphics.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
    for (int division = 1; division < 4; ++division) {
        const auto x = plot.getX() + plot.getWidth() * static_cast<float>(division) / 4.0f;
        graphics.drawVerticalLine(juce::roundToInt(x), plot.getY(), plot.getBottom());
    }

    const auto sampleRate = std::max(8000.0, processor_.getSampleRate());
    const auto previewFrequency = static_cast<float>(sampleRate / 256.0);
    pas1::dsp::WavetableOscillator oscillator;
    oscillator.reset();
    juce::Path wave;
    constexpr int sampleCount = 256;
    for (int sample = 0; sample < sampleCount; ++sample) {
        const auto value = oscillator.nextSample(
            previewFrequency, static_cast<float>(sampleRate),
            static_cast<pas1::dsp::Waveform>(waveform), scan);
        const auto x = plot.getX() + plot.getWidth()
                                     * static_cast<float>(sample)
                                     / static_cast<float>(sampleCount - 1);
        const auto y = plot.getCentreY() - value * plot.getHeight() * 0.43f;
        if (sample == 0)
            wave.startNewSubPath(x, y);
        else
            wave.lineTo(x, y);
    }
    graphics.setColour(juce::Colour{accentColour});
    graphics.strokePath(wave, juce::PathStrokeType{1.8f});
}

void FilterGraphs::update() {
    const auto sampleRate = static_cast<float>(processor_.getSampleRate() > 0.0
                                                   ? processor_.getSampleRate()
                                                   : 44100.0);
    const auto cutoff = parameterValue(processor_, "filtercutoff", 1000.0f);
    const auto resonance = parameterValue(processor_, "filterresonance", 0.15f);
    const auto type = std::clamp(juce::roundToInt(
        parameterValue(processor_, "filtertype", 0.0f)), 0, 3);
    const auto envelopeAmount = parameterValue(processor_, "filterenvamount", 0.0f);
    const auto attack = parameterValue(processor_, "filterattack", 0.01f);
    const auto decay = parameterValue(processor_, "filterdecay", 0.2f);
    const auto sustain = parameterValue(processor_, "filtersustain", 0.7f);
    const auto release = parameterValue(processor_, "filterrelease", 0.2f);

    const auto changed = std::abs(cutoff - cutoffHz_) > 0.01f
                         || std::abs(resonance - resonance_) > 0.0001f
                         || std::abs(sampleRate - sampleRate_) > 0.1f
                         || type != filterType_
                         || std::abs(envelopeAmount - envelopeAmount_) > 0.0001f
                         || std::abs(attack - attack_) > 0.0001f
                         || std::abs(decay - decay_) > 0.0001f
                         || std::abs(sustain - sustain_) > 0.0001f
                         || std::abs(release - release_) > 0.0001f;
    if (!changed)
        return;

    cutoffHz_ = cutoff;
    resonance_ = resonance;
    sampleRate_ = sampleRate;
    filterType_ = type;
    envelopeAmount_ = envelopeAmount;
    attack_ = attack;
    decay_ = decay;
    sustain_ = sustain;
    release_ = release;
    calculateResponse();
    repaint();
}

juce::Rectangle<float> FilterGraphs::responsePlotBounds() const noexcept {
    const auto bounds = getLocalBounds().toFloat();
    auto plot = juce::Rectangle<float>{bounds.getX() + 42.0f, bounds.getY() + 25.0f,
                                       std::max(1.0f, bounds.getWidth() - 54.0f),
                                       bounds.getHeight() * 0.44f};
    plot.setHeight(std::min(plot.getHeight(), bounds.getHeight() * 0.46f));
    return plot;
}

FilterGraphs::EnvelopeLayout FilterGraphs::envelopeLayout() const noexcept {
    const auto bounds = getLocalBounds().toFloat();
    const auto titleY = bounds.getY() + bounds.getHeight() * 0.61f;
    EnvelopeLayout layout;
    layout.bounds = {bounds.getX() + 18.0f, titleY + 14.0f,
                     std::max(1.0f, bounds.getWidth() - 30.0f),
                     std::max(1.0f, bounds.getBottom() - titleY - 20.0f)};

    const auto total = std::max(0.001f, attack_ + decay_ + release_);
    const auto timedWidth = layout.bounds.getWidth() * (1.0f - envelopeHoldFraction);
    layout.attackX = layout.bounds.getX() + timedWidth * attack_ / total;
    layout.decayX = layout.attackX + timedWidth * decay_ / total;
    layout.releaseX = layout.decayX
                      + layout.bounds.getWidth() * envelopeHoldFraction;
    layout.sustainY = layout.bounds.getBottom()
                      - std::clamp(sustain_, 0.0f, 1.0f) * layout.bounds.getHeight();
    return layout;
}

void FilterGraphs::calculateResponse() noexcept {
    constexpr double pi = 3.14159265358979323846;
    const auto sampleRate = static_cast<double>(std::max(sampleRate_, 8000.0f));
    const auto maxFrequency = std::min(20000.0, sampleRate * 0.45);
    const auto minFrequency = std::min(20.0, maxFrequency * 0.1);
    const auto cutoff = std::clamp(static_cast<double>(cutoffHz_), 10.0, maxFrequency);
    const auto g = std::tan(pi * cutoff / sampleRate);
    const auto q = static_cast<double>(
        pas1::dsp::StateVariableFilter::resonanceToQ(resonance_));
    const auto k = 1.0 / q;
    const auto logRange = std::log(maxFrequency / minFrequency);

    for (std::size_t index = 0; index < responseDb_.size(); ++index) {
        const auto position = static_cast<double>(index)
                              / static_cast<double>(responseDb_.size() - 1);
        const auto frequency = minFrequency * std::exp(logRange * position);
        const auto u = std::tan(pi * frequency / sampleRate) / g;
        const auto denominator = std::sqrt((1.0 - u * u) * (1.0 - u * u) + (k * u) * (k * u));
        double magnitude = 0.0;
        switch (static_cast<pas1::dsp::FilterType>(filterType_)) {
            case pas1::dsp::FilterType::lowPass: magnitude = 1.0 / denominator; break;
            case pas1::dsp::FilterType::highPass: magnitude = u * u / denominator; break;
            case pas1::dsp::FilterType::bandPass: magnitude = u / denominator; break;
            case pas1::dsp::FilterType::notch:
                magnitude = std::abs(1.0 - u * u) / denominator;
                break;
        }
        const auto decibels = 20.0 * std::log10(std::max(magnitude, 1.0e-5));
        responseDb_[index] = static_cast<float>(std::clamp(decibels, -60.0, 24.0));
    }
}

void FilterGraphs::beginParameterGesture(const char* id) {
    if (gestureParameterCount_ >= gestureParameters_.size())
        return;
    if (auto* parameter = processor_.parameters.getParameter(id)) {
        parameter->beginChangeGesture();
        gestureParameters_[gestureParameterCount_++] = parameter;
    }
}

void FilterGraphs::endParameterGestures() {
    for (std::size_t index = 0; index < gestureParameterCount_; ++index) {
        if (auto* parameter = gestureParameters_[index])
            parameter->endChangeGesture();
        gestureParameters_[index] = nullptr;
    }
    gestureParameterCount_ = 0;
    activeHandle_ = DragHandle::none;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void FilterGraphs::setParameterValue(const char* id, float value) {
    if (auto* parameter = processor_.parameters.getParameter(id)) {
        const auto range = parameter->getNormalisableRange();
        const auto bounded = std::clamp(value, range.start, range.end);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(bounded));
    }
}

void FilterGraphs::mouseDown(const juce::MouseEvent& event) {
    const auto position = event.position;
    if (responsePlotBounds().contains(position)) {
        endParameterGestures();
        activeHandle_ = DragHandle::filterResponse;
        beginParameterGesture("filtercutoff");
        beginParameterGesture("filterresonance");
        updateFromMouse(position);
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        return;
    }

    const auto layout = envelopeLayout();
    if (!layout.bounds.contains(position))
        return;

    const std::array<std::pair<DragHandle, juce::Point<float>>, 3> handles{{
        {DragHandle::attack, {layout.attackX, layout.bounds.getY()}},
        {DragHandle::decaySustain, {layout.decayX, layout.sustainY}},
        {DragHandle::release, {layout.releaseX, layout.sustainY}}
    }};
    auto closest = DragHandle::none;
    auto closestDistanceSquared = 24.0f * 24.0f;
    for (const auto& [handle, point] : handles) {
        const auto distance = position - point;
        const auto distanceSquared = distance.x * distance.x + distance.y * distance.y;
        if (distanceSquared < closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closest = handle;
        }
    }
    if (closest == DragHandle::none)
        return;

    endParameterGestures();
    activeHandle_ = closest;
    if (closest == DragHandle::attack) {
        beginParameterGesture("filterattack");
    } else if (closest == DragHandle::decaySustain) {
        beginParameterGesture("filterdecay");
        beginParameterGesture("filtersustain");
    } else {
        beginParameterGesture("filterrelease");
    }
    updateFromMouse(position);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void FilterGraphs::mouseDrag(const juce::MouseEvent& event) {
    if (activeHandle_ != DragHandle::none)
        updateFromMouse(event.position);
}

void FilterGraphs::mouseUp(const juce::MouseEvent&) {
    endParameterGestures();
}

void FilterGraphs::updateFromMouse(juce::Point<float> position) {
    if (activeHandle_ == DragHandle::filterResponse) {
        const auto plot = responsePlotBounds();
        const auto sampleRate = std::max(static_cast<double>(sampleRate_), 8000.0);
        const auto minFrequency = std::min(20.0, sampleRate * 0.0025);
        const auto maxFrequency = std::min(20000.0, sampleRate * 0.45);
        const auto fraction = std::clamp(
            (position.x - plot.getX()) / std::max(1.0f, plot.getWidth()), 0.0f, 1.0f);
        const auto frequency = minFrequency
                               * std::exp(std::log(maxFrequency / minFrequency) * fraction);
        const auto resonance = std::clamp(
            1.0f - (position.y - plot.getY()) / std::max(1.0f, plot.getHeight()),
            0.0f, 1.0f);
        setParameterValue("filtercutoff", static_cast<float>(frequency));
        setParameterValue("filterresonance", resonance);
        update();
        return;
    }

    const auto layout = envelopeLayout();
    const auto fraction = std::clamp(
        (position.x - layout.bounds.getX()) / std::max(1.0f, layout.bounds.getWidth()),
        0.02f, 0.98f);
    const auto timedWidthFraction = 1.0f - envelopeHoldFraction;
    const auto readValue = [this](const char* id, float fallback) {
        if (const auto* value = processor_.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);
        return fallback;
    };

    switch (activeHandle_) {
        case DragHandle::attack: {
            const auto timedFraction = std::clamp(
                fraction / timedWidthFraction, 0.02f, 0.98f);
            const auto remaining = decay_ + release_;
            setParameterValue("filterattack",
                              timedFraction * remaining / (1.0f - timedFraction));
            break;
        }
        case DragHandle::decaySustain: {
            const auto timedFraction = std::clamp(
                fraction / timedWidthFraction, 0.02f, 0.98f);
            const auto nextDecay =
                (timedFraction * (attack_ + release_) - attack_)
                / std::max(0.02f, 1.0f - timedFraction);
            const auto sustain = std::clamp(
                (layout.bounds.getBottom() - position.y)
                    / std::max(1.0f, layout.bounds.getHeight()),
                0.0f, 1.0f);
            setParameterValue("filterdecay", nextDecay);
            setParameterValue("filtersustain", sustain);
            break;
        }
        case DragHandle::release: {
            const auto timedFraction = std::clamp(
                (fraction - envelopeHoldFraction) / timedWidthFraction,
                0.02f, 0.98f);
            const auto attack = readValue("filterattack", attack_);
            const auto decay = readValue("filterdecay", decay_);
            setParameterValue("filterrelease",
                              (attack + decay) * (1.0f - timedFraction) / timedFraction);
            break;
        }
        case DragHandle::none:
        case DragHandle::filterResponse:
            return;
    }
    update();
}

void FilterGraphs::paint(juce::Graphics& graphics) {
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour(juce::Colour{panelColour});
    graphics.fillRoundedRectangle(bounds, 8.0f);

    auto header = bounds.reduced(12.0f, 5.0f);
    graphics.setColour(juce::Colour{textColour});
    graphics.setFont(juce::FontOptions{9.5f, juce::Font::bold});
    graphics.drawText(pas1::ui::utf8String("ЧАСТОТНЫЙ ОТКЛИК • ")
                          + filterName(std::max(0, filterType_)),
                      header.removeFromTop(14.0f).toNearestInt(),
                      juce::Justification::centredLeft);

    const auto graphSampleRate = std::max(static_cast<double>(sampleRate_), 8000.0);
    const auto minFrequency = std::min(20.0, graphSampleRate * 0.0025);
    const auto maxFrequency = std::min(20000.0, graphSampleRate * 0.45);
    const auto logRange = std::log(maxFrequency / minFrequency);
    auto responsePlot = responsePlotBounds();
    const auto yForDb = [&responsePlot](float db) {
        return responsePlot.getBottom() - (db + 60.0f) / 84.0f * responsePlot.getHeight();
    };

    graphics.setFont(juce::FontOptions{8.0f});
    for (const auto db : {-48.0f, -24.0f, 0.0f, 12.0f, 24.0f}) {
        const auto y = yForDb(db);
        graphics.setColour(juce::Colour{gridColour});
        graphics.drawHorizontalLine(juce::roundToInt(y), responsePlot.getX(), responsePlot.getRight());
        graphics.setColour(juce::Colour{mutedColour});
        graphics.drawText(juce::String{juce::roundToInt(db)}, 4,
                          juce::roundToInt(y - 5.0f), 34, 10,
                          juce::Justification::centredRight);
    }

    constexpr std::array<double, 5> tickFrequencies{20.0, 100.0, 1000.0, 10000.0, 20000.0};
    constexpr std::array<const char*, 5> tickLabels{"20", "100", "1k", "10k", "20k"};
    for (std::size_t tick = 0; tick < tickFrequencies.size(); ++tick) {
        const auto frequency = std::clamp(tickFrequencies[tick], minFrequency, maxFrequency);
        const auto x = responsePlot.getX()
                       + responsePlot.getWidth()
                             * static_cast<float>(std::log(frequency / minFrequency) / logRange);
        graphics.setColour(juce::Colour{gridColour});
        graphics.drawVerticalLine(juce::roundToInt(x), responsePlot.getY(), responsePlot.getBottom());
        if (tick == 0 || tick == 2 || tick == 4) {
            graphics.setColour(juce::Colour{mutedColour});
            graphics.drawText(tickLabels[tick], juce::roundToInt(x - 15.0f),
                              juce::roundToInt(responsePlot.getBottom() + 1.0f), 30, 10,
                              juce::Justification::centred);
        }
    }

    juce::Path response;
    for (std::size_t index = 0; index < responseDb_.size(); ++index) {
        const auto x = responsePlot.getX() + responsePlot.getWidth()
                       * static_cast<float>(index) / static_cast<float>(responseDb_.size() - 1);
        const auto y = yForDb(responseDb_[index]);
        if (index == 0)
            response.startNewSubPath(x, y);
        else
            response.lineTo(x, y);
    }
    graphics.setColour(juce::Colour{accentColour});
    graphics.strokePath(response, juce::PathStrokeType{1.7f});

    const auto clampedCutoff = std::clamp(static_cast<double>(cutoffHz_),
                                          minFrequency, maxFrequency);
    const auto cutoffFraction = static_cast<float>(
        std::log(clampedCutoff / minFrequency) / logRange);
    const auto cutoffX = responsePlot.getX() + responsePlot.getWidth() * cutoffFraction;
    const auto responseIndex = cutoffFraction
                               * static_cast<float>(responseDb_.size() - 1);
    const auto lowerIndex = static_cast<std::size_t>(std::clamp(
        juce::roundToInt(responseIndex), 0,
        static_cast<int>(responseDb_.size() - 1)));
    const auto cutoffY = yForDb(responseDb_[lowerIndex]);
    graphics.setColour(juce::Colour{accentColour}.withAlpha(0.5f));
    graphics.drawVerticalLine(juce::roundToInt(cutoffX), responsePlot.getY(),
                              responsePlot.getBottom());
    graphics.setColour(juce::Colour{textColour});
    graphics.fillEllipse(cutoffX - 4.5f, cutoffY - 4.5f, 9.0f, 9.0f);
    graphics.setColour(juce::Colour{accentColour});
    graphics.drawEllipse(cutoffX - 5.0f, cutoffY - 5.0f, 10.0f, 10.0f, 1.5f);

    const auto envelopeTitleY = bounds.getY() + bounds.getHeight() * 0.61f;
    const auto depthText = pas1::ui::utf8String("ОГИБАЮЩАЯ ФИЛЬТРА • ГЛУБИНА ")
                           + juce::String{envelopeAmount_ * 4.0f, 1}
                           + pas1::ui::utf8String(" ОКТ.");
    graphics.setColour(juce::Colour{textColour});
    graphics.setFont(juce::FontOptions{8.5f, juce::Font::bold});
    graphics.drawText(depthText, 12, juce::roundToInt(envelopeTitleY), getWidth() - 24, 12,
                      juce::Justification::centredLeft);

    const auto layout = envelopeLayout();
    auto envelopePlot = layout.bounds;
    if (envelopePlot.getHeight() < 14.0f)
        return;
    graphics.setColour(juce::Colour{gridColour});
    graphics.drawHorizontalLine(juce::roundToInt(envelopePlot.getBottom()),
                                envelopePlot.getX(), envelopePlot.getRight());
    juce::Path envelope;
    envelope.startNewSubPath(envelopePlot.getX(), envelopePlot.getBottom());
    envelope.lineTo(layout.attackX, envelopePlot.getY());
    envelope.lineTo(layout.decayX, layout.sustainY);
    envelope.lineTo(layout.releaseX, layout.sustainY);
    envelope.lineTo(envelopePlot.getRight(), envelopePlot.getBottom());
    graphics.setColour(juce::Colour{accentColour});
    graphics.strokePath(envelope, juce::PathStrokeType{1.4f});

    const auto drawHandle = [&graphics](float x, float y) {
        graphics.setColour(juce::Colour{accentColour}.withAlpha(0.24f));
        graphics.fillEllipse(x - 7.0f, y - 7.0f, 14.0f, 14.0f);
        graphics.setColour(juce::Colour{panelColour});
        graphics.fillEllipse(x - 3.5f, y - 3.5f, 7.0f, 7.0f);
        graphics.setColour(juce::Colour{accentColour});
        graphics.drawEllipse(x - 3.5f, y - 3.5f, 7.0f, 7.0f, 1.3f);
    };
    drawHandle(layout.attackX, envelopePlot.getY());
    drawHandle(layout.decayX, layout.sustainY);
    drawHandle(layout.releaseX, layout.sustainY);
}
