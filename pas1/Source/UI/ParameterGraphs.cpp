#include "ParameterGraphs.h"
#include "Utf8Text.h"

#include "../DSP/Oscillator.h"
#include "../DSP/StateVariableFilter.h"
#include "../DSP/WavetableOscillator.h"

#include <algorithm>
#include <array>
#include <cmath>

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
    constexpr std::array<const char*, 4> names{"СИНУС", "ТРЕУГОЛЬНИК", "ПИЛА", "ИМПУЛЬС"};
    return pas1::ui::utf8String(names[static_cast<std::size_t>(std::clamp(waveform, 0, 3))]);
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
        juce::roundToInt(parameterValue(processor_, "waveform", 0.0f)), 0, 3);
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
    auto responsePlot = juce::Rectangle<float>{bounds.getX() + 42.0f, bounds.getY() + 25.0f,
                                                bounds.getWidth() - 54.0f, bounds.getHeight() * 0.44f};
    responsePlot.setHeight(std::min(responsePlot.getHeight(), bounds.getHeight() * 0.46f));
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

    const auto envelopeTitleY = bounds.getY() + bounds.getHeight() * 0.61f;
    const auto depthText = pas1::ui::utf8String("ОГИБАЮЩАЯ ФИЛЬТРА • ГЛУБИНА ")
                           + juce::String{envelopeAmount_ * 4.0f, 1}
                           + pas1::ui::utf8String(" ОКТ.");
    graphics.setColour(juce::Colour{textColour});
    graphics.setFont(juce::FontOptions{8.5f, juce::Font::bold});
    graphics.drawText(depthText, 12, juce::roundToInt(envelopeTitleY), getWidth() - 24, 12,
                      juce::Justification::centredLeft);

    auto envelopePlot = juce::Rectangle<float>{bounds.getX() + 18.0f,
                                                envelopeTitleY + 14.0f,
                                                bounds.getWidth() - 30.0f,
                                                bounds.getBottom() - envelopeTitleY - 20.0f};
    if (envelopePlot.getHeight() < 14.0f)
        return;
    const auto total = std::max(0.001f, attack_ + decay_ + release_);
    const auto hold = total * 0.22f;
    const auto full = attack_ + decay_ + hold + release_;
    const auto attackX = envelopePlot.getX() + envelopePlot.getWidth() * attack_ / full;
    const auto decayX = attackX + envelopePlot.getWidth() * decay_ / full;
    const auto releaseX = envelopePlot.getRight()
                          - envelopePlot.getWidth() * release_ / full;
    const auto sustainY = envelopePlot.getBottom()
                          - std::clamp(sustain_, 0.0f, 1.0f) * envelopePlot.getHeight();
    graphics.setColour(juce::Colour{gridColour});
    graphics.drawHorizontalLine(juce::roundToInt(envelopePlot.getBottom()),
                                envelopePlot.getX(), envelopePlot.getRight());
    juce::Path envelope;
    envelope.startNewSubPath(envelopePlot.getX(), envelopePlot.getBottom());
    envelope.lineTo(attackX, envelopePlot.getY());
    envelope.lineTo(decayX, sustainY);
    envelope.lineTo(releaseX, sustainY);
    envelope.lineTo(envelopePlot.getRight(), envelopePlot.getBottom());
    graphics.setColour(juce::Colour{accentColour});
    graphics.strokePath(envelope, juce::PathStrokeType{1.4f});
}
