#pragma once

namespace pas1::dsp {

enum class Waveform {
    sine = 0,
    triangle,
    saw,
    square,
    pulse,
    reverseSaw,
    organ,
    hollow,
    formant,
    bell,
    metallic,
    clipped,
    count
};

class Oscillator {
public:
    void reset(double phase = 0.0) noexcept;
    float nextSample(float frequency, float sampleRate, Waveform waveform,
                     float pulseWidth = 0.5f) noexcept;
    double phase() const noexcept { return phase_; }

private:
    static float polyBlep(double phase, double phaseIncrement) noexcept;
    double phase_ = 0.0;
};

} // namespace pas1::dsp
