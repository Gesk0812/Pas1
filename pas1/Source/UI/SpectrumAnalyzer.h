#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

#include <array>
#include <complex>

class SpectrumAnalyzer final : public juce::Component {
public:
    explicit SpectrumAnalyzer(Pas1AudioProcessor& processor) : processor_(processor) {}

    void update();
    void paint(juce::Graphics&) override;

private:
    static constexpr std::size_t fftSize = 1024;
    static constexpr std::size_t spectrumSize = fftSize / 2;

    Pas1AudioProcessor& processor_;
    std::array<float, fftSize> samples_{};
    std::array<std::complex<float>, fftSize> fftBuffer_{};
    std::array<float, spectrumSize> magnitudes_{};
};
