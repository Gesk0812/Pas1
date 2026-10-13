#include "Tuning.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace pas1::dsp {
namespace {
constexpr double a4Frequency = 440.0;
constexpr double semitonesPerOctave = 12.0;
}

float temperamentRatio(int midiNoteNumber, int temperament, int rootNote) noexcept {
    if (temperament <= static_cast<int>(Temperament::equal))
        return 1.0f;

    constexpr std::array<double, 12> justRatios{
        1.0, 16.0 / 15.0, 9.0 / 8.0, 6.0 / 5.0, 5.0 / 4.0, 4.0 / 3.0,
        45.0 / 32.0, 3.0 / 2.0, 8.0 / 5.0, 5.0 / 3.0, 9.0 / 5.0, 15.0 / 8.0};
    constexpr std::array<double, 12> pythagoreanRatios{
        1.0, 256.0 / 243.0, 9.0 / 8.0, 32.0 / 27.0, 81.0 / 64.0, 4.0 / 3.0,
        729.0 / 512.0, 3.0 / 2.0, 128.0 / 81.0, 27.0 / 16.0, 16.0 / 9.0, 243.0 / 128.0};

    const auto positiveModulo = [](int value) {
        return (value % 12 + 12) % 12;
    };
    const auto root = positiveModulo(rootNote);
    const auto pitchClass = positiveModulo(midiNoteNumber);
    const auto interval = positiveModulo(pitchClass - root);
    const auto equalRatio = std::exp2(static_cast<double>(interval) / semitonesPerOctave);

    switch (static_cast<Temperament>(temperament)) {
        case Temperament::justIntonation:
            return static_cast<float>(justRatios[static_cast<std::size_t>(interval)] / equalRatio);
        case Temperament::pythagorean:
            return static_cast<float>(
                pythagoreanRatios[static_cast<std::size_t>(interval)] / equalRatio);
        case Temperament::nineteenToneEqual: {
            const auto nineteenToneStep = static_cast<int>(
                std::round(static_cast<double>(interval) * 19.0 / semitonesPerOctave));
            const auto nineteenToneRatio =
                std::exp2(static_cast<double>(nineteenToneStep) / 19.0);
            return static_cast<float>(nineteenToneRatio / equalRatio);
        }
        case Temperament::equal:
        default:
            return 1.0f;
    }
}

float tunedMidiFrequency(int midiNoteNumber, int octaveShift, int transposeSemitones,
                         float fineTuneCents, int temperament, int rootNote) noexcept {
    if (!std::isfinite(fineTuneCents))
        fineTuneCents = 0.0f;
    fineTuneCents = std::clamp(fineTuneCents, -1200.0f, 1200.0f);
    const auto shiftedNote = std::clamp(
        midiNoteNumber + transposeSemitones + octaveShift * 12, -128, 255);
    const auto equalTemperedFrequency =
        a4Frequency * std::exp2(static_cast<double>(shiftedNote - 69) / semitonesPerOctave);
    const auto tuning = temperamentRatio(shiftedNote, temperament, rootNote);
    const auto fineTuneRatio = std::exp2(static_cast<double>(fineTuneCents) / 1200.0);
    const auto frequency = equalTemperedFrequency * tuning * fineTuneRatio;
    return std::isfinite(frequency) && frequency > 0.0
               ? static_cast<float>(frequency)
               : 0.0f;
}

} // namespace pas1::dsp
