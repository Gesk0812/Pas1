#include "WavetableOscillator.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace pas1::dsp {
namespace {
constexpr double pi = 3.14159265358979323846;

float harmonicSample(int waveform, double phase, int maximumHarmonic) noexcept {
    double sum = 0.0;
    switch (waveform) {
        case 0:
            return static_cast<float>(std::sin(2.0 * pi * phase));
        case 1:
            for (int harmonic = 1; harmonic <= maximumHarmonic; harmonic += 2) {
                const auto sign = ((harmonic - 1) / 2) % 2 == 0 ? 1.0 : -1.0;
                sum += sign * std::sin(2.0 * pi * harmonic * phase)
                       / (static_cast<double>(harmonic) * harmonic);
            }
            return static_cast<float>(sum * (8.0 / (pi * pi)));
        case 2:
            for (int harmonic = 1; harmonic <= maximumHarmonic; ++harmonic)
                sum += std::sin(2.0 * pi * harmonic * phase) / harmonic;
            return static_cast<float>(-2.0 * sum / pi);
        case 3:
            for (int harmonic = 1; harmonic <= maximumHarmonic; harmonic += 2)
                sum += std::sin(2.0 * pi * harmonic * phase) / harmonic;
            return static_cast<float>(4.0 * sum / pi);
        case 4: // Narrow pulse, approximately 25% duty cycle.
            for (int harmonic = 1; harmonic <= maximumHarmonic; ++harmonic)
                sum += std::sin(2.0 * pi * harmonic * phase)
                       * std::sin(pi * harmonic * 0.25) / harmonic;
            return static_cast<float>(2.0 * sum);
        case 5: // Reverse saw.
            for (int harmonic = 1; harmonic <= maximumHarmonic; ++harmonic)
                sum += std::sin(2.0 * pi * harmonic * phase) / harmonic;
            return static_cast<float>(2.0 * sum / pi);
        case 6: // Organ-like additive wave.
            for (int harmonic = 1; harmonic <= maximumHarmonic; ++harmonic)
                sum += std::sin(2.0 * pi * harmonic * phase)
                       / std::pow(static_cast<double>(harmonic), 0.72);
            return static_cast<float>(sum);
        case 7: // Hollow, odd-harmonic reed.
            for (int harmonic = 1; harmonic <= maximumHarmonic; ++harmonic) {
                if (harmonic % 2 == 0)
                    continue;
                const auto weight = harmonic % 4 == 1 ? 1.0 : -0.58;
                sum += weight * std::sin(2.0 * pi * harmonic * phase) / harmonic;
            }
            return static_cast<float>(sum);
        case 8: // Vowel-like formant emphasis.
            for (int harmonic = 1; harmonic <= maximumHarmonic; ++harmonic) {
                const auto distance = static_cast<double>(harmonic) - 4.0;
                const auto formant = std::exp(-(distance * distance) / 4.5);
                sum += formant * std::sin(2.0 * pi * harmonic * phase) / harmonic;
            }
            return static_cast<float>(sum * 2.3);
        case 9: // Bell-like inharmonic partial series.
        case 10: {
            constexpr std::array<double, 7> bellRatios{1.0, 2.0, 3.01, 4.18, 5.43, 6.7, 8.1};
            constexpr std::array<double, 7> metalRatios{1.0, 1.41, 2.76, 4.93, 6.6, 8.21, 10.7};
            constexpr std::array<double, 7> partialWeights{1.0, 0.58, 0.36, 0.25, 0.17, 0.12, 0.08};
            const auto& ratios = waveform == 9 ? bellRatios : metalRatios;
            for (std::size_t index = 0; index < ratios.size(); ++index)
                if (ratios[index] <= maximumHarmonic)
                    sum += partialWeights[index]
                           * std::sin(2.0 * pi * ratios[index] * phase);
            return static_cast<float>(sum);
        }
        case 11: // Soft-clipped sine with a controlled upper spectrum.
            sum = std::sin(2.0 * pi * phase)
                  + 0.34 * std::sin(6.0 * pi * phase)
                  + 0.16 * std::sin(10.0 * pi * phase)
                  + 0.08 * std::sin(14.0 * pi * phase);
            return static_cast<float>(sum);
        default:
            return 0.0f;
    }
}
} // namespace

const WavetableOscillator::Bank& WavetableOscillator::getBank() {
    static const auto bank = [] {
        auto result = std::make_unique<Bank>();
        for (int waveform = 0; waveform < waveformCount; ++waveform) {
            for (int level = 0; level < mipLevels; ++level) {
                const auto maximumHarmonic = 1 << level;
                auto& table = (*result)[static_cast<std::size_t>(waveform)]
                                       [static_cast<std::size_t>(level)];
                float peak = 0.0f;
                for (int sample = 0; sample < tableSize; ++sample) {
                    const auto phase = static_cast<double>(sample) / tableSize;
                    auto& value = table[static_cast<std::size_t>(sample)];
                    value = harmonicSample(waveform, phase, maximumHarmonic);
                    peak = std::max(peak, std::abs(value));
                }
                if (peak > 0.0f)
                    for (auto& value : table)
                        value /= peak;
            }
        }
        return result;
    }();
    return *bank;
}

void WavetableOscillator::initialize() {
    static_cast<void>(getBank());
}

void WavetableOscillator::reset(double phase) noexcept {
    phase_ = std::isfinite(phase) ? phase - std::floor(phase) : 0.0;
}

float WavetableOscillator::nextSample(float frequency, float sampleRate, Waveform waveform) noexcept {
    return nextSample(frequency, sampleRate, waveform, 0.0f);
}

float WavetableOscillator::nextSample(float frequency, float sampleRate, Waveform waveform,
                                      float scan) noexcept {
    if (!std::isfinite(frequency) || !std::isfinite(sampleRate) || sampleRate <= 0.0f)
        return 0.0f;

    const auto safeFrequency = std::clamp(frequency, 0.0f, sampleRate * 0.45f);
    const auto maximumHarmonics = static_cast<int>(
        std::clamp(sampleRate / (2.0f * std::max(safeFrequency, 1.0f)), 1.0f, 1024.0f));
    int level = 0;
    while (level + 1 < mipLevels && (1 << (level + 1)) <= maximumHarmonics)
        ++level;

    constexpr auto waveCount = static_cast<float>(waveformCount);
    const auto baseWaveform = static_cast<float>(
        std::clamp(static_cast<int>(waveform), 0, waveformCount - 1));
    auto tablePosition = baseWaveform
                         + std::clamp(std::isfinite(scan) ? scan : 0.0f, 0.0f, 1.0f)
                               * waveCount;
    tablePosition -= std::floor(tablePosition / waveCount) * waveCount;
    const auto lowerWaveform = static_cast<int>(tablePosition);
    const auto upperWaveform = (lowerWaveform + 1) % waveformCount;
    const auto waveformFraction = tablePosition - static_cast<float>(lowerWaveform);
    const auto& bank = getBank();
    const auto& lowerTable = bank[static_cast<std::size_t>(lowerWaveform)]
                                 [static_cast<std::size_t>(level)];
    const auto& upperTable = bank[static_cast<std::size_t>(upperWaveform)]
                                 [static_cast<std::size_t>(level)];
    const auto position = phase_ * tableSize;
    const auto index = static_cast<int>(position);
    const auto fraction = static_cast<float>(position - index);
    const auto nextIndex = (index + 1) % tableSize;
    const auto lowerSample = lowerTable[static_cast<std::size_t>(index)] * (1.0f - fraction)
                             + lowerTable[static_cast<std::size_t>(nextIndex)] * fraction;
    const auto upperSample = upperTable[static_cast<std::size_t>(index)] * (1.0f - fraction)
                             + upperTable[static_cast<std::size_t>(nextIndex)] * fraction;
    const auto sample = lowerSample * (1.0f - waveformFraction) + upperSample * waveformFraction;

    phase_ += static_cast<double>(safeFrequency / sampleRate);
    if (phase_ >= 1.0)
        phase_ -= std::floor(phase_);

    return std::isfinite(sample) ? sample : 0.0f;
}

} // namespace pas1::dsp
