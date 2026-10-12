#include "WavetableOscillator.h"

#include <algorithm>
#include <cmath>

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
        default:
            return 0.0f;
    }
}
} // namespace

const WavetableOscillator::Bank& WavetableOscillator::getBank() {
    static const Bank bank = [] {
        Bank result{};
        for (int waveform = 0; waveform < 4; ++waveform) {
            for (int level = 0; level < mipLevels; ++level) {
                const auto maximumHarmonic = 1 << level;
                auto& table = result[static_cast<std::size_t>(waveform)][static_cast<std::size_t>(level)];
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
    return bank;
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

    const auto baseWaveform = static_cast<float>(std::clamp(static_cast<int>(waveform), 0, 3));
    auto tablePosition = baseWaveform + std::clamp(std::isfinite(scan) ? scan : 0.0f, 0.0f, 1.0f) * 4.0f;
    tablePosition -= std::floor(tablePosition / 4.0f) * 4.0f;
    const auto lowerWaveform = static_cast<int>(tablePosition);
    const auto upperWaveform = (lowerWaveform + 1) % 4;
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
