#pragma once

namespace pas1::dsp {

enum class Temperament : int {
    equal = 0,
    justIntonation,
    pythagorean,
    nineteenToneEqual
};

float temperamentRatio(int midiNoteNumber, int temperament, int rootNote) noexcept;
float tunedMidiFrequency(int midiNoteNumber, int octaveShift, int transposeSemitones,
                         float fineTuneCents, int temperament, int rootNote) noexcept;

} // namespace pas1::dsp
