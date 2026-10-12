#include "SpectrumAnalyzer.h"
#include "Utf8Text.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
const juce::Colour analyzerBackground{0xff24160f};
const juce::Colour analyzerGrid{0xff745332};
const juce::Colour analyzerAccent{0xffd8b36a};
const juce::Colour analyzerText{0xffbba483};
}

void SpectrumAnalyzer::update() {
    processor_.copyRecentAudioSamples(samples_.data(), static_cast<int>(samples_.size()));
    for (std::size_t index = 0; index < fftSize; ++index) {
        const auto window = 0.5f - 0.5f * std::cos(
            juce::MathConstants<float>::twoPi * static_cast<float>(index)
            / static_cast<float>(fftSize - 1));
        fftBuffer_[index] = {samples_[index] * window, 0.0f};
    }

    for (std::size_t index = 1, reversed = 0; index < fftSize; ++index) {
        auto bit = fftSize >> 1;
        for (; (reversed & bit) != 0; bit >>= 1)
            reversed ^= bit;
        reversed ^= bit;
        if (index < reversed)
            std::swap(fftBuffer_[index], fftBuffer_[reversed]);
    }

    for (std::size_t length = 2; length <= fftSize; length <<= 1) {
        const auto angle = -juce::MathConstants<float>::twoPi / static_cast<float>(length);
        const std::complex<float> step{std::cos(angle), std::sin(angle)};
        for (std::size_t start = 0; start < fftSize; start += length) {
            auto twiddle = std::complex<float>{1.0f, 0.0f};
            const auto halfLength = length / 2;
            for (std::size_t offset = 0; offset < halfLength; ++offset) {
                const auto even = fftBuffer_[start + offset];
                const auto odd = fftBuffer_[start + offset + halfLength] * twiddle;
                fftBuffer_[start + offset] = even + odd;
                fftBuffer_[start + offset + halfLength] = even - odd;
                twiddle *= step;
            }
        }
    }

    for (std::size_t bin = 0; bin < spectrumSize; ++bin) {
        const auto magnitude = std::abs(fftBuffer_[bin]) * (4.0f / static_cast<float>(fftSize));
        magnitudes_[bin] = std::max(magnitude, magnitudes_[bin] * 0.88f);
    }
    repaint();
}

void SpectrumAnalyzer::paint(juce::Graphics& graphics) {
    graphics.setColour(analyzerBackground);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    graphics.setColour(analyzerText);
    graphics.setFont(juce::FontOptions{9.0f, juce::Font::bold});
    graphics.drawText(pas1::ui::utf8String("СПЕКТР ВЫХОДА"), 8, 4, 130, 12,
                      juce::Justification::centredLeft);

    const auto plot = getLocalBounds().reduced(8)
                          .withTrimmedTop(16)
                          .withTrimmedLeft(24)
                          .withTrimmedBottom(12);
    if (plot.getWidth() < 2 || plot.getHeight() < 2)
        return;

    const auto sampleRate = std::max(8000.0f,
        processor_.visualSampleRate.load(std::memory_order_relaxed));
    const auto minFrequency = 20.0f;
    const auto maxFrequency = std::max(minFrequency * 2.0f,
                                       std::min(20000.0f, sampleRate * 0.48f));
    const auto logRange = std::log(maxFrequency / minFrequency);

    graphics.setColour(analyzerGrid);
    for (int line = 1; line <= 2; ++line) {
        const auto y = plot.getY() + plot.getHeight() * line / 3;
        graphics.drawHorizontalLine(y, static_cast<float>(plot.getX()),
                                    static_cast<float>(plot.getRight()));
    }
    for (const auto fraction : {0.25f, 0.5f, 0.75f}) {
        const auto x = plot.getX() + juce::roundToInt(
            static_cast<float>(plot.getWidth()) * fraction);
        graphics.drawVerticalLine(x, static_cast<float>(plot.getY()),
                                  static_cast<float>(plot.getBottom()));
    }

    graphics.setFont(juce::FontOptions{8.0f});
    for (const auto [level, label] : {std::pair{0.0f, "0"},
                                      std::pair{-36.0f, "-36"},
                                      std::pair{-72.0f, "-72"}}) {
        const auto fraction = (level + 72.0f) / 72.0f;
        const auto y = static_cast<float>(plot.getBottom())
                       - fraction * static_cast<float>(plot.getHeight());
        graphics.setColour(analyzerText);
        graphics.drawText(label, 0, juce::roundToInt(y - 5.0f), 21, 10,
                          juce::Justification::centredRight);
    }

    for (const auto [frequency, label] : {std::pair{20.0f, "20"},
                                          std::pair{1000.0f, "1k"},
                                          std::pair{20000.0f, "20k"}}) {
        const auto boundedFrequency = std::clamp(frequency, minFrequency, maxFrequency);
        const auto fraction = std::log(boundedFrequency / minFrequency) / logRange;
        const auto x = plot.getX() + juce::roundToInt(
            static_cast<float>(plot.getWidth()) * fraction);
        graphics.setColour(analyzerText);
        graphics.drawText(label, x - 18, plot.getBottom() + 1, 36, 10,
                          juce::Justification::centred);
    }

    juce::Path spectrum;
    for (int x = 0; x < plot.getWidth(); ++x) {
        const auto position = static_cast<float>(x)
                              / static_cast<float>(std::max(1, plot.getWidth() - 1));
        const auto frequency = minFrequency * std::exp(logRange * position);
        const auto rawBin = juce::roundToInt(frequency * static_cast<float>(fftSize) / sampleRate);
        const auto bin = static_cast<std::size_t>(std::clamp(
            rawBin, 1, static_cast<int>(spectrumSize) - 1));
        const auto decibels = 20.0f * std::log10(std::max(magnitudes_[bin], 1.0e-5f));
        const auto level = std::clamp((decibels + 72.0f) / 72.0f, 0.0f, 1.0f);
        const auto y = static_cast<float>(plot.getBottom())
                       - level * static_cast<float>(plot.getHeight());
        const auto plotX = static_cast<float>(plot.getX() + x);
        if (x == 0)
            spectrum.startNewSubPath(plotX, y);
        else
            spectrum.lineTo(plotX, y);
    }
    graphics.setColour(analyzerAccent);
    graphics.strokePath(spectrum, juce::PathStrokeType{1.5f});
}
