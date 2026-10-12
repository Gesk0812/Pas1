#pragma once

#include "Oscillator.h"

#include <array>

namespace pas1::dsp {

class WavetableOscillator {
public:
    static void initialize();

    void reset(double phase = 0.0) noexcept;
    float nextSample(float frequency, float sampleRate, Waveform waveform) noexcept;
    float nextSample(float frequency, float sampleRate, Waveform waveform, float scan) noexcept;
    double phase() const noexcept { return phase_; }

private:
    static constexpr int tableSize = 2048;
    static constexpr int mipLevels = 11;
    using Table = std::array<float, tableSize>;
    using WaveTables = std::array<Table, mipLevels>;
    using Bank = std::array<WaveTables, 4>;

    static const Bank& getBank();

    double phase_ = 0.0;
};

} // namespace pas1::dsp
