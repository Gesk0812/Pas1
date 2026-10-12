#include "DSP/AmpEnvelope.h"
#include "DSP/EffectsChain.h"
#include "DSP/MorphMatrix.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/NotePriorityStack.h"
#include "DSP/Portamento.h"
#include "DSP/SoundDNA.h"
#include "DSP/LowFrequencyOscillator.h"
#include "DSP/Oscillator.h"
#include "DSP/MetallicTine.h"
#include "DSP/AncientInstrument.h"
#include "DSP/StateVariableFilter.h"
#include "DSP/WavetableOscillator.h"
#include "Generation/SoundGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testOscillator() {
    constexpr float sampleRate = 48000.0f;
    constexpr float frequency = 440.0f;
    for (const auto waveform : {pas1::dsp::Waveform::sine, pas1::dsp::Waveform::triangle,
                                pas1::dsp::Waveform::saw, pas1::dsp::Waveform::square}) {
        pas1::dsp::Oscillator oscillator;
        double energy = 0.0;
        for (int i = 0; i < 48000; ++i) {
            const auto sample = oscillator.nextSample(frequency, sampleRate, waveform);
            expect(std::isfinite(sample), "oscillator samples must be finite");
            expect(std::abs(sample) <= 1.01f, "oscillator output must stay in range");
            energy += static_cast<double>(sample) * sample;
        }
        expect(energy > 1000.0, "each waveform must generate a non-silent signal");
        expect(oscillator.phase() >= 0.0 && oscillator.phase() < 1.0, "phase must wrap into [0, 1)");
    }

    pas1::dsp::Oscillator invalidInput;
    expect(invalidInput.nextSample(std::numeric_limits<float>::quiet_NaN(), sampleRate,
                                  pas1::dsp::Waveform::sine) == 0.0f,
           "non-finite frequency must be handled safely");

    pas1::dsp::Oscillator pulseOscillator;
    double pulseMean = 0.0;
    for (int i = 0; i < 48000; ++i)
        pulseMean += pulseOscillator.nextSample(480.0f, sampleRate, pas1::dsp::Waveform::square, 0.25f);
    pulseMean /= 48000.0;
    expect(std::abs(pulseMean + 0.5) < 0.08,
           "pulse width must change the square-wave duty cycle");
}

void testMetallicTine() {
    constexpr float sampleRate = 48000.0f;
    pas1::dsp::MetallicTine tine;
    tine.trigger(440.0f, sampleRate, 0.8f, 1.7f, 0.6f, 0.75f, 0.65f);

    double earlyEnergy = 0.0;
    double lateEnergy = 0.0;
    for (int index = 0; index < 240000; ++index) {
        const auto sample = tine.nextSample(440.0f, sampleRate);
        expect(std::isfinite(sample), "metallic tine output must remain finite");
        expect(std::abs(sample) <= 1.0f, "metallic tine output must remain bounded");
        if (index < 2400)
            earlyEnergy += static_cast<double>(sample) * sample;
        if (index >= 216000)
            lateEnergy += static_cast<double>(sample) * sample;
    }
    expect(earlyEnergy > 1.0, "metallic tine must produce a non-silent struck tone");
    expect(lateEnergy < earlyEnergy * 0.2,
           "metallic tine partials must decay instead of sustaining indefinitely");

    tine.trigger(std::numeric_limits<float>::quiet_NaN(), sampleRate, 0.8f, 1.7f,
                 0.6f, 0.75f, 0.65f);
    expect(tine.nextSample(440.0f, sampleRate) == 0.0f,
           "invalid trigger frequency must leave the tine silent");
}

void testAncientInstrumentVoices() {
    constexpr float sampleRate = 48000.0f;
    constexpr std::array<pas1::dsp::AncientInstrument, 4> instruments{
        pas1::dsp::AncientInstrument::aulos,
        pas1::dsp::AncientInstrument::lyre,
        pas1::dsp::AncientInstrument::bronzeGong,
        pas1::dsp::AncientInstrument::foundMetal
    };
    std::array<std::array<float, 256>, instruments.size()> signatures{};

    for (std::size_t instrumentIndex = 0; instrumentIndex < instruments.size();
         ++instrumentIndex) {
        pas1::dsp::AncientInstrumentVoice voice;
        voice.trigger(instruments[instrumentIndex], 220.0f, sampleRate,
                      0.7f, 2.6f, 0.55f, 0.4f,
                      static_cast<std::uint32_t>(instrumentIndex + 11));
        double energy = 0.0;
        for (int sampleIndex = 0; sampleIndex < 4096; ++sampleIndex) {
            const auto sample = voice.nextSample(220.0f, sampleRate);
            expect(std::isfinite(sample), "ancient instrument samples must stay finite");
            expect(std::abs(sample) <= 1.001f, "ancient instrument output must stay bounded");
            energy += static_cast<double>(sample) * sample;
            if (sampleIndex < static_cast<int>(signatures[instrumentIndex].size()))
                signatures[instrumentIndex][static_cast<std::size_t>(sampleIndex)] = sample;
        }
        expect(energy > 0.01, "each ancient instrument must produce an audible signal");
    }

    for (std::size_t first = 0; first < signatures.size(); ++first) {
        for (std::size_t second = first + 1; second < signatures.size(); ++second) {
            double difference = 0.0;
            for (std::size_t sample = 0; sample < signatures[first].size(); ++sample)
                difference += std::abs(signatures[first][sample] - signatures[second][sample]);
            expect(difference > 0.1, "ancient instrument timbres must be distinct");
        }
    }

    pas1::dsp::AncientInstrumentVoice disabled;
    disabled.trigger(pas1::dsp::AncientInstrument::off, 220.0f, sampleRate,
                     0.7f, 2.6f, 0.55f, 0.4f, 1);
    expect(disabled.nextSample(220.0f, sampleRate) == 0.0f,
           "the off setting must remain silent");
    disabled.trigger(pas1::dsp::AncientInstrument::lyre,
                     std::numeric_limits<float>::quiet_NaN(), sampleRate,
                     0.7f, 2.6f, 0.55f, 0.4f, 1);
    expect(disabled.nextSample(220.0f, sampleRate) == 0.0f,
           "invalid ancient instrument triggers must safely reset the voice");
}

void testEnvelope() {
    pas1::dsp::AmpEnvelope envelope;
    envelope.setSampleRate(1000.0);
    envelope.setParameters({0.01f, 0.02f, 0.5f, 0.03f});
    envelope.noteOn();

    for (int i = 0; i < 10; ++i)
        envelope.nextSample();
    expect(envelope.level() == 1.0f, "attack must reach its peak");

    for (int i = 0; i < 20; ++i)
        envelope.nextSample();
    expect(std::abs(envelope.level() - 0.5f) < 0.001f, "decay must reach sustain level");

    envelope.noteOff();
    for (int i = 0; i < 29; ++i)
        envelope.nextSample();
    expect(envelope.isActive(), "release must remain active until its duration ends");
    envelope.nextSample();
    expect(!envelope.isActive(), "release must finish and deactivate the envelope");
    expect(envelope.level() == 0.0f, "released envelope must end at zero");

    envelope.setParameters({0.0f, 0.0f, 0.8f, 0.0f});
    envelope.noteOn();
    expect(std::abs(envelope.nextSample() - 0.8f) < 0.001f, "zero-length stages must advance safely");
    envelope.noteOff();
    expect(envelope.nextSample() == 0.0f, "zero-length release must stop immediately");

    pas1::dsp::AmpEnvelope retriggered;
    retriggered.setSampleRate(1000.0);
    retriggered.setParameters({0.1f, 0.1f, 0.4f, 0.1f});
    retriggered.noteOn();
    for (int sample = 0; sample < 50; ++sample)
        retriggered.nextSample();
    expect(std::abs(retriggered.level() - 0.5f) < 0.02f,
           "linear attack must reach the expected midpoint");
    const auto levelAtRelease = retriggered.level();
    retriggered.noteOff();
    for (int sample = 0; sample < 50; ++sample)
        retriggered.nextSample();
    expect(std::abs(retriggered.level() - levelAtRelease * 0.5f) < 0.03f,
           "release must decay from the note's current level");
    expect(retriggered.isActive(), "release must stay active until its configured time ends");
    retriggered.noteOn();
    for (int sample = 0; sample < 50; ++sample)
        retriggered.nextSample();
    expect(std::abs(retriggered.level() - 0.5f) < 0.02f,
           "a retrigger during release must begin a fresh attack");
}

void testWavetableOscillator() {
    pas1::dsp::WavetableOscillator::initialize();
    constexpr float sampleRate = 48000.0f;
    for (const auto waveform : {pas1::dsp::Waveform::sine, pas1::dsp::Waveform::triangle,
                                pas1::dsp::Waveform::saw, pas1::dsp::Waveform::square}) {
        pas1::dsp::WavetableOscillator oscillator;
        double energy = 0.0;
        for (int i = 0; i < 48000; ++i) {
            const auto sample = oscillator.nextSample(440.0f, sampleRate, waveform);
            expect(std::isfinite(sample), "wavetable samples must remain finite");
            expect(std::abs(sample) <= 1.05f, "wavetable samples must stay in range");
            energy += static_cast<double>(sample) * sample;
        }
        expect(energy > 1000.0, "each wavetable must produce a non-silent signal");
    }

    pas1::dsp::WavetableOscillator baseTable;
    pas1::dsp::WavetableOscillator scannedTable;
    double scanDifference = 0.0;
    for (int i = 0; i < 4096; ++i) {
        const auto base = baseTable.nextSample(220.0f, sampleRate, pas1::dsp::Waveform::saw, 0.0f);
        const auto scanned = scannedTable.nextSample(220.0f, sampleRate, pas1::dsp::Waveform::saw, 0.25f);
        scanDifference += std::abs(base - scanned);
    }
    expect(scanDifference > 100.0, "wavetable scan must crossfade to a different table");

    for (const auto waveform : {pas1::dsp::Waveform::sine, pas1::dsp::Waveform::triangle,
                                pas1::dsp::Waveform::saw, pas1::dsp::Waveform::square}) {
        pas1::dsp::WavetableOscillator nearNyquist;
        for (int sample = 0; sample < 4096; ++sample) {
            const auto value = nearNyquist.nextSample(
                sampleRate * 0.42f, sampleRate, waveform, 0.5f);
            expect(std::isfinite(value) && std::abs(value) <= 1.05f,
                   "high-pitch wavetable playback must remain finite and bounded");
        }
    }

    constexpr std::size_t fftSize = 4096;
    constexpr int fundamentalBin = 1700;
    const auto highFrequency = sampleRate * static_cast<float>(fundamentalBin)
                               / static_cast<float>(fftSize);
    pas1::dsp::WavetableOscillator bandLimitedSaw;
    std::array<float, fftSize> highPitchSamples{};
    for (auto& sample : highPitchSamples)
        sample = bandLimitedSaw.nextSample(
            highFrequency, sampleRate, pas1::dsp::Waveform::saw, 0.0f);
    const auto binMagnitude = [&highPitchSamples](int bin) {
        double real = 0.0;
        double imaginary = 0.0;
        for (std::size_t sample = 0; sample < highPitchSamples.size(); ++sample) {
            const auto phase = 2.0 * 3.14159265358979323846 * bin * sample
                               / static_cast<double>(highPitchSamples.size());
            real += highPitchSamples[sample] * std::cos(phase);
            imaginary -= highPitchSamples[sample] * std::sin(phase);
        }
        return static_cast<float>(2.0 * std::hypot(real, imaginary)
                                  / static_cast<double>(highPitchSamples.size()));
    };
    const auto fundamentalMagnitude = binMagnitude(fundamentalBin);
    const auto secondHarmonicAliasMagnitude = binMagnitude(
        static_cast<int>(fftSize) - fundamentalBin * 2);
    expect(fundamentalMagnitude > 0.2f,
           "near-Nyquist saw playback must retain its fundamental");
    expect(secondHarmonicAliasMagnitude < fundamentalMagnitude * 0.1f,
           "wavetable mip selection must suppress folded high harmonics near Nyquist");
}

void testFilter() {
    constexpr float sampleRate = 48000.0f;
    const auto measureGain = [sampleRate](float cutoff, float resonance,
                                          pas1::dsp::FilterType type, float frequency) {
        pas1::dsp::StateVariableFilter filter;
        filter.prepare(sampleRate);
        filter.setParameters(cutoff, resonance, type);
        const auto period = std::max(1, static_cast<int>(std::lround(sampleRate / frequency)));
        const auto warmupSamples = std::max(4096, period * 8);
        const auto measureSamples = period * 4;
        double squareSum = 0.0;
        for (int sample = 0; sample < warmupSamples + measureSamples; ++sample) {
            const auto input = std::sin(2.0 * 3.14159265358979323846
                                        * frequency * sample / sampleRate);
            const auto output = filter.processSample(static_cast<float>(input));
            if (sample >= warmupSamples)
                squareSum += static_cast<double>(output) * output;
        }
        return static_cast<float>(std::sqrt(2.0 * squareSum / measureSamples));
    };

    for (const auto type : {pas1::dsp::FilterType::lowPass, pas1::dsp::FilterType::highPass,
                            pas1::dsp::FilterType::bandPass, pas1::dsp::FilterType::notch}) {
        pas1::dsp::StateVariableFilter filter;
        filter.prepare(sampleRate);
        filter.setParameters(1200.0f, 0.8f, type);
        for (int i = 0; i < 48000; ++i) {
            const auto input = i == 0 ? 1.0f : 0.0f;
            const auto output = filter.processSample(input);
            expect(std::isfinite(output), "filter must not produce NaN or Inf");
            expect(std::abs(output) <= 16.0f, "filter output must remain bounded");
        }
    }

    pas1::dsp::StateVariableFilter lowPass;
    pas1::dsp::StateVariableFilter highPass;
    lowPass.prepare(sampleRate);
    highPass.prepare(sampleRate);
    lowPass.setParameters(100.0f, 0.2f, pas1::dsp::FilterType::lowPass);
    highPass.setParameters(100.0f, 0.2f, pas1::dsp::FilterType::highPass);
    double lowPassEnergy = 0.0;
    double highPassEnergy = 0.0;
    for (int i = 0; i < 48000; ++i) {
        const auto input = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * 4000.0 * i / sampleRate));
        const auto low = lowPass.processSample(input);
        const auto high = highPass.processSample(input);
        lowPassEnergy += static_cast<double>(low) * low;
        highPassEnergy += static_cast<double>(high) * high;
    }
    expect(highPassEnergy > lowPassEnergy * 4.0,
           "high-pass filter must attenuate a high-frequency tone less than low-pass");

    const auto lowPassAtCutoff = measureGain(1200.0f, 0.0f,
        pas1::dsp::FilterType::lowPass, 1200.0f);
    const auto highPassAtCutoff = measureGain(1200.0f, 0.0f,
        pas1::dsp::FilterType::highPass, 1200.0f);
    const auto bandPassAtCutoff = measureGain(1200.0f, 0.0f,
        pas1::dsp::FilterType::bandPass, 1200.0f);
    const auto notchAtCutoff = measureGain(1200.0f, 0.0f,
        pas1::dsp::FilterType::notch, 1200.0f);
    expect(std::abs(lowPassAtCutoff - 0.7071f) < 0.035f,
           "zero resonance low-pass must have a Butterworth-like cutoff response");
    expect(std::abs(highPassAtCutoff - 0.7071f) < 0.035f,
           "zero resonance high-pass must have a Butterworth-like cutoff response");
    expect(std::abs(bandPassAtCutoff - 0.7071f) < 0.035f,
           "zero resonance band-pass must have a controlled center response");
    expect(notchAtCutoff < 0.02f,
           "notch filter must strongly cancel the cutoff frequency");

    constexpr std::array<float, 6> resonanceSteps{0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f};
    float previousGain = 0.0f;
    for (const auto resonance : resonanceSteps) {
        const auto q = pas1::dsp::StateVariableFilter::resonanceToQ(resonance);
        const auto gain = measureGain(1200.0f, resonance,
            pas1::dsp::FilterType::lowPass, 1200.0f);
        expect(std::isfinite(q) && q >= 0.707f && q <= 12.001f,
               "resonance-to-Q mapping must remain within its stable bounds");
        expect(gain > previousGain,
               "increasing resonance must produce a progressively stronger cutoff peak");
        previousGain = gain;
    }
    expect(std::abs(previousGain - 12.0f) < 0.2f,
           "maximum resonance must remain strong and finite");

    for (const auto legacyValue : {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.99f, 1.0f}) {
        const auto oldQ = 0.5f + std::clamp(legacyValue, 0.1f, 0.99f) * 11.5f;
        const auto migrated = pas1::dsp::StateVariableFilter::migrateLegacyResonance(legacyValue);
        const auto restoredQ = pas1::dsp::StateVariableFilter::resonanceToQ(migrated);
        expect(std::abs(oldQ - restoredQ) < 0.002f,
               "legacy resonance migration must preserve the previous filter Q");
    }
}

void testLfo() {
    pas1::dsp::LowFrequencyOscillator first;
    pas1::dsp::LowFrequencyOscillator second;
    first.reset(12345u);
    second.reset(12345u);
    for (const auto waveform : {pas1::dsp::LfoWaveform::sine, pas1::dsp::LfoWaveform::triangle,
                                pas1::dsp::LfoWaveform::saw, pas1::dsp::LfoWaveform::square,
                                pas1::dsp::LfoWaveform::sampleAndHold,
                                pas1::dsp::LfoWaveform::randomSmooth}) {
        first.reset(12345u);
        second.reset(12345u);
        for (int i = 0; i < 50000; ++i) {
            const auto a = first.nextSample(2.0f, 48000.0f, waveform);
            const auto b = second.nextSample(2.0f, 48000.0f, waveform);
            expect(std::isfinite(a) && std::abs(a) <= 1.0f,
                   "LFO output must remain finite and in range");
            expect(a == b, "seeded LFO sequences must be repeatable");
        }
    }
}

void testEffects() {
    constexpr float sampleRate = 48000.0f;
    pas1::dsp::EffectsChain effects;
    effects.prepare(sampleRate);

    std::vector<float> left(48000, 0.0f);
    std::vector<float> right(48000, 0.0f);
    left[0] = 1.0f;
    right[0] = 1.0f;
    float* channels[] = {left.data(), right.data()};

    pas1::dsp::EffectsParameters parameters;
    parameters.delayEnabled = true;
    parameters.delayMix = 0.8f;
    parameters.delayTimeSeconds = 0.02f;
    parameters.delayFeedback = 0.25f;
    effects.process(channels, 2, static_cast<int>(left.size()), parameters);

    double delayedEnergy = 0.0;
    for (int i = 900; i < 1100; ++i)
        delayedEnergy += std::abs(left[static_cast<std::size_t>(i)]);
    expect(delayedEnergy > 0.1, "delay must return an audible delayed copy");
    for (std::size_t i = 0; i < left.size(); ++i)
        expect(std::isfinite(left[i]) && std::isfinite(right[i]),
               "effects chain must not produce NaN or Inf");

    const auto limited = pas1::dsp::EffectsChain::limitSample(4.0f, true, 0.9f);
    expect(limited <= 1.0f && limited > 0.9f,
           "enabled output limiter must softly contain peaks");
    expect(pas1::dsp::EffectsChain::limitSample(4.0f, false, 0.9f) == 1.0f,
           "disabled limiter must still protect the output from hard overflow");

    for (int effectIndex = 0; effectIndex < 6; ++effectIndex) {
        for (int bypassMode = 0; bypassMode < 2; ++bypassMode) {
            pas1::dsp::EffectsChain bypassChain;
            bypassChain.prepare(sampleRate);
            std::array<float, 256> dryLeft{};
            std::array<float, 256> dryRight{};
            std::array<float, 256> wetLeft{};
            std::array<float, 256> wetRight{};
            for (std::size_t sample = 0; sample < dryLeft.size(); ++sample) {
                const auto value = 0.35f * std::sin(
                    2.0 * 3.14159265358979323846 * 317.0 * sample / sampleRate)
                    + 0.05f * std::cos(
                        2.0 * 3.14159265358979323846 * 83.0 * sample / sampleRate);
                dryLeft[sample] = dryRight[sample] = value;
                wetLeft[sample] = wetRight[sample] = value;
            }
            float* bypassChannels[] = {wetLeft.data(), wetRight.data()};
            pas1::dsp::EffectsParameters bypassParameters;
            bypassParameters.distortionEnabled = false;
            bypassParameters.chorusEnabled = false;
            bypassParameters.phaserEnabled = false;
            bypassParameters.delayEnabled = false;
            bypassParameters.reverbEnabled = false;
            bypassParameters.eqEnabled = false;
            bypassParameters.limiterEnabled = false;
            bypassParameters.stereoWidth = 1.0f;

            const auto effectEnabled = bypassMode == 0;
            const auto mix = effectEnabled ? 0.0f : 1.0f;
            switch (effectIndex) {
                case 0:
                    bypassParameters.distortionEnabled = effectEnabled;
                    bypassParameters.distortionMix = mix;
                    break;
                case 1:
                    bypassParameters.chorusEnabled = effectEnabled;
                    bypassParameters.chorusMix = mix;
                    break;
                case 2:
                    bypassParameters.phaserEnabled = effectEnabled;
                    bypassParameters.phaserMix = mix;
                    break;
                case 3:
                    bypassParameters.delayEnabled = effectEnabled;
                    bypassParameters.delayMix = mix;
                    break;
                case 4:
                    bypassParameters.reverbEnabled = effectEnabled;
                    bypassParameters.reverbMix = mix;
                    break;
                case 5:
                    bypassParameters.eqEnabled = effectEnabled;
                    bypassParameters.eqMix = mix;
                    break;
                default: break;
            }

            bypassChain.process(bypassChannels, 2, static_cast<int>(wetLeft.size()),
                                bypassParameters);
            for (std::size_t sample = 0; sample < wetLeft.size(); ++sample) {
                expect(std::abs(wetLeft[sample] - dryLeft[sample]) < 1.0e-5f
                           && std::abs(wetRight[sample] - dryRight[sample]) < 1.0e-5f,
                       "effect bypass and zero-wet settings must preserve the dry signal");
            }
        }
    }

    pas1::dsp::EffectsChain widthEffects;
    widthEffects.prepare(sampleRate);
    float widthLeft[] = {1.0f};
    float widthRight[] = {0.0f};
    float* widthChannels[] = {widthLeft, widthRight};
    pas1::dsp::EffectsParameters widthParameters;
    widthParameters.stereoWidth = 0.0f;
    widthEffects.process(widthChannels, 2, 1, widthParameters);
    expect(std::abs(widthLeft[0] - 0.5f) < 0.0001f
               && std::abs(widthRight[0] - 0.5f) < 0.0001f,
           "zero stereo width produces a centered mono signal");
}

void testModulationMatrix() {
    using Matrix = pas1::dsp::ModulationMatrix;
    Matrix::SourceValues sources{};
    Matrix::Routes routes{};
    sources[static_cast<std::size_t>(pas1::dsp::ModulationSource::lfo1)] = -0.5f;
    routes[0] = {pas1::dsp::ModulationSource::lfo1,
                 pas1::dsp::ModulationDestination::pitch, 0.4f,
                 pas1::dsp::ModulationPolarity::bipolar, true};
    routes[1] = {pas1::dsp::ModulationSource::lfo1,
                 pas1::dsp::ModulationDestination::pitch, 0.2f,
                 pas1::dsp::ModulationPolarity::unipolar, true};
    routes[2] = {pas1::dsp::ModulationSource::lfo1,
                 pas1::dsp::ModulationDestination::pan, 1.0f,
                 pas1::dsp::ModulationPolarity::bipolar, false};
    const auto output = Matrix::evaluate(sources, routes);
    expect(std::abs(output[static_cast<std::size_t>(pas1::dsp::ModulationDestination::pitch)]
                    + 0.15f) < 0.0001f,
           "multiple modulation routes must sum with bipolar/unipolar polarity");
    expect(output[static_cast<std::size_t>(pas1::dsp::ModulationDestination::pan)] == 0.0f,
           "disabled modulation routes must not affect a destination");

    routes[0].amount = 1.0f;
    routes[0].polarity = pas1::dsp::ModulationPolarity::unipolar;
    routes[1].enabled = false;
    const auto clipped = Matrix::evaluate(sources, routes);
    expect(clipped[static_cast<std::size_t>(pas1::dsp::ModulationDestination::pitch)] == 0.25f,
           "unipolar LFO source must map bipolar input to [0, 1]");

    routes = {};
    sources[static_cast<std::size_t>(pas1::dsp::ModulationSource::lfo1)] = 1.0f;
    routes[0] = {pas1::dsp::ModulationSource::lfo1,
                 pas1::dsp::ModulationDestination::pitch, 1.0f,
                 pas1::dsp::ModulationPolarity::bipolar, true};
    routes[1] = routes[0];
    const auto summed = Matrix::evaluate(sources, routes);
    expect(summed[static_cast<std::size_t>(pas1::dsp::ModulationDestination::pitch)] == 1.0f,
           "multiple routes to one target must sum and clamp to the normalized range");
    sources[static_cast<std::size_t>(pas1::dsp::ModulationSource::lfo1)] =
        std::numeric_limits<float>::quiet_NaN();
    const auto nonFinite = Matrix::evaluate(sources, routes);
    expect(nonFinite[static_cast<std::size_t>(pas1::dsp::ModulationDestination::pitch)] == 0.0f,
           "non-finite modulation sources must not escape into DSP");

    pas1::dsp::ModulationAccumulator accumulator;
    accumulator.add(output);
    accumulator.add(clipped);
    const auto average = accumulator.average();
    expect(std::abs(average[static_cast<std::size_t>(pas1::dsp::ModulationDestination::pitch)]
                    - 0.05f) < 0.0001f,
           "global modulation accumulator must average active voice samples");
    accumulator.reset();
    expect(accumulator.average()[static_cast<std::size_t>(
               pas1::dsp::ModulationDestination::pitch)] == 0.0f,
           "empty modulation accumulator must return zero");
}

void testPortamento() {
    pas1::dsp::Portamento glide;
    glide.reset(440.0f);
    glide.glideTo(880.0f, 4);
    expect(glide.isGliding(), "portamento must remain active until its duration ends");
    expect(std::abs(glide.nextSample() - 440.0f * std::pow(2.0f, 0.25f)) < 0.02f,
           "portamento must glide exponentially in pitch");
    const auto interruptedFrequency = glide.currentFrequency();
    glide.glideTo(220.0f, 2);
    expect(std::abs(glide.currentFrequency() - interruptedFrequency) < 0.001f,
           "retargeting portamento must continue from its current frequency");
    glide.nextSample();
    expect(glide.isGliding(), "retargeted portamento must use the new duration");
    expect(std::abs(glide.nextSample() - 220.0f) < 0.001f && !glide.isGliding(),
           "portamento must reach the target exactly at the end of its duration");
    glide.glideTo(330.0f, 0);
    expect(glide.nextSample() == 330.0f, "zero-duration portamento must jump directly to target");
}

void testNotePriorityStack() {
    pas1::dsp::NotePriorityStack notes;
    expect(notes.size() == 0 && notes.mostRecent() == nullptr,
           "empty mono note stack must have no priority note");
    expect(!notes.noteOn({1, 60, 0.6f}),
           "the first held key must not be marked as a legato transition");
    expect(notes.noteOn({1, 67, 0.8f}),
           "pressing another key while one is held must mark legato");
    expect(notes.mostRecent() != nullptr && notes.mostRecent()->note == 67,
           "the latest held key must have mono priority");
    expect(notes.noteOff(1, 67) && notes.mostRecent()->note == 60,
           "releasing the priority key must restore the previous held key");
    expect(!notes.noteOff(2, 60),
           "note-offs on a different MIDI channel must not remove a held key");
    expect(notes.noteOff(1, 60) && notes.size() == 0,
           "releasing the final held key must empty the note stack");

    notes.noteOn({1, 60, 0.4f});
    notes.noteOn({1, 64, 0.5f});
    notes.noteOn({1, 60, 0.9f});
    expect(notes.size() == 2 && notes.mostRecent()->note == 60
               && std::abs(notes.mostRecent()->velocity - 0.9f) < 0.0001f,
           "retriggering a held pitch must update its velocity and priority without duplication");
}

void testMorphMatrix() {
    using pas1::dsp::MorphMatrix;
    using pas1::dsp::MorphParameter;
    MorphMatrix matrix;
    MorphMatrix::Values a{}, b{}, c{}, d{}, current{};
    a.fill(0.0f);
    b.fill(1.0f);
    c.fill(2.0f);
    d.fill(3.0f);
    const auto cutoff = static_cast<std::size_t>(MorphParameter::filterCutoff);
    a[cutoff] = 100.0f;
    b[cutoff] = 1000.0f;
    c[cutoff] = 10000.0f;
    d[cutoff] = 1000.0f;
    matrix.captureCorner(0, a);
    matrix.captureCorner(1, b);
    matrix.captureCorner(2, c);
    matrix.captureCorner(3, d);
    expect(matrix.isEnabled(), "capturing a morph corner enables the matrix");

    current.fill(0.0f);
    auto output = matrix.process(0.0f, 0.0f, current, 48000.0, 512);
    expect(output[0] == 0.0f, "morph A is the lower-left corner");
    output = matrix.process(1.0f, 1.0f, current, 48000.0, 512);
    expect(output[0] > 0.0f && output[0] < 3.0f,
           "morph output transitions smoothly instead of jumping");

    for (int step = 0; step < 80; ++step)
        output = matrix.process(0.5f, 0.5f, current, 48000.0, 512);
    expect(std::abs(output[0] - 1.5f) < 0.02f,
           "bilinear morphing reaches the expected center value");
    expect(std::abs(output[cutoff] - 1000.0f) < 2.0f,
           "cutoff uses logarithmic interpolation at the center");

    matrix.setLocked(0, true);
    current[0] = 0.77f;
    output = matrix.process(1.0f, 1.0f, current, 48000.0, 512);
    expect(std::abs(output[0] - current[0]) < 0.0001f,
           "a locked morph target follows its current parameter value");

    MorphMatrix first, second;
    first.captureCorner(0, a);
    second.captureCorner(0, a);
    first.captureCorner(1, b);
    second.captureCorner(1, b);
    first.randomizeCorners(12345);
    second.randomizeCorners(12345);
    expect(first.getCorner(2) == second.getCorner(2),
           "morph randomization is reproducible from a seed");
}

void testSoundDNA() {
    using pas1::dsp::SoundDNA;
    using pas1::dsp::SoundDNAFeature;
    using pas1::generation::ParameterValue;
    const std::vector<ParameterValue> bright{
        {"filtercutoff", 0.95f}, {"wavetablescan", 0.7f}, {"osc1level", 0.8f},
        {"osc2level", 0.35f}, {"osc3level", 0.25f}, {"fmamount", 0.55f},
        {"ringamount", 0.3f}, {"noiselevel", 0.1f}, {"sublevel", 0.1f},
        {"unisonvoices", 0.5f}, {"unisondetune", 0.5f}, {"lfo1amount", 0.3f},
        {"stereowidth", 0.9f}, {"attack", 0.1f}, {"decay", 0.3f},
        {"sustain", 0.6f}, {"release", 0.2f}, {"distortionmix", 0.1f},
        {"filterdrive", 0.1f}, {"reverbmix", 0.35f}};
    auto dark = bright;
    for (auto& parameter : dark) {
        if (parameter.id == "filtercutoff")
            parameter.normalizedValue = 0.1f;
        if (parameter.id == "stereowidth")
            parameter.normalizedValue = 0.2f;
        if (parameter.id == "sublevel")
            parameter.normalizedValue = 0.8f;
    }

    const auto brightProfile = SoundDNA::analyze(bright);
    const auto darkProfile = SoundDNA::analyze(dark);
    expect(brightProfile.get(SoundDNAFeature::brightness)
               > darkProfile.get(SoundDNAFeature::brightness),
           "Sound DNA brightness follows the filter and spectral controls");
    expect(brightProfile.get(SoundDNAFeature::stereoWidth)
               > darkProfile.get(SoundDNAFeature::stereoWidth),
           "Sound DNA stereo width follows the width parameter");
    expect(SoundDNA::distance(brightProfile, brightProfile) == 0.0f,
           "identical Sound DNA profiles have zero distance");
    const auto middle = SoundDNA::blend(brightProfile, darkProfile, 0.5f);
    expect(std::abs(middle.get(SoundDNAFeature::brightness)
                    - (brightProfile.get(SoundDNAFeature::brightness)
                       + darkProfile.get(SoundDNAFeature::brightness)) * 0.5f) < 0.0001f,
           "DNA blending interpolates feature values");
    const auto controls = SoundDNA::toRecipeControls(brightProfile);
    expect(std::abs(controls.harmonicComplexity
                    - brightProfile.get(SoundDNAFeature::harmonicComplexity)) < 0.0001f,
           "DNA profiles map to sound recipe controls");
}

void testSoundGenerator() {
    using pas1::generation::ParameterValue;
    const std::vector<ParameterValue> base{
        {"filtercutoff", 0.5f, 0.5f, 20.0f, 20000.0f, 0.25f, 0, false},
        {"filterdrive", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"sublevel", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"attack", 0.4f, 0.2f, 0.001f, 5.0f, 0.35f, 0, false},
        {"unisonvoices", 0.0f, 0.0f, 1.0f, 7.0f, 1.0f, 7, true},
        {"waveform", 0.0f, 0.0f, 0.0f, 3.0f, 1.0f, 4, true},
        {"lfo1amount", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"chorusmix", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"master", 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, false}};
    const std::vector<ParameterValue> musicBoxBase{
        {"musicboxmix", 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"musicboxbrightness", 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"musicboxdecay", 0.25f, 0.0f, 0.08f, 8.0f, 1.0f, 0, false},
        {"musicboxresonance", 0.35f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"musicboxinharmonicity", 0.4f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"musicboxstrike", 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"master", 0.6f, 0.6f, 0.0f, 1.0f, 1.0f, 0, false}};

    pas1::generation::RecipeControls controls;
    controls.brightness = 0.5f;
    controls.density = 0.4f;
    controls.movement = 0.5f;
    controls.aggression = 0.25f;
    controls.space = 0.4f;
    controls.harmonicComplexity = 0.65f;
    controls.bassWeight = 0.35f;
    controls.stereoWidth = 0.5f;
    const auto recipe = pas1::generation::SoundGenerator::createRecipe(
        base, "Warm evolving analog bass with metallic attack, moderate distortion and a wide stereo tail.",
        controls);
    const auto findValue = [](const auto& values, const std::string& id) -> const ParameterValue* {
        const auto found = std::find_if(values.begin(), values.end(),
                                        [&id](const auto& value) { return value.id == id; });
        return found == values.end() ? nullptr : &*found;
    };
    expect(findValue(recipe, "sublevel")->normalizedValue > 0.0f,
           "bass recipe should add sub weight");
    expect(findValue(recipe, "filterdrive")->normalizedValue > 0.0f,
           "moderate distortion recipe should add drive");
    expect(findValue(recipe, "chorusmix")->normalizedValue > 0.0f,
           "wide recipe should add stereo modulation");
    expect(findValue(recipe, "master")->normalizedValue == findValue(base, "master")->normalizedValue,
           "recipe should leave the master level unchanged");
    expect(recipe.size() == base.size(), "recipe should preserve unknown parameter entries");

    const auto unknownRecipe = pas1::generation::SoundGenerator::createRecipe(
        base, "xyzzy florp quux", controls);
    expect(unknownRecipe.size() == base.size(), "unknown recipe words must be safe");

    const auto musicBoxRecipe = pas1::generation::SoundGenerator::createRecipe(
        musicBoxBase, "bright music box with metallic tines", controls);
    const auto findMusicBoxValue = [&findValue](const auto& values, const std::string& id) {
        return findValue(values, id);
    };
    expect(findMusicBoxValue(musicBoxRecipe, "musicboxmix")->normalizedValue > 0.8f,
           "music-box recipe should enable its dedicated tine engine");
    expect(findMusicBoxValue(musicBoxRecipe, "musicboxinharmonicity")->normalizedValue
               > findMusicBoxValue(musicBoxBase, "musicboxinharmonicity")->normalizedValue,
           "music-box recipe should raise the inharmonic tine character");
    const std::vector<ParameterValue> ancientBase{
        {"ancientinstrument", 0.0f, 0.0f, 0.0f, 4.0f, 1.0f, 5, true},
        {"ancientmix", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, false},
        {"ancientbrightness", 0.68f, 0.68f, 0.0f, 1.0f, 1.0f, 0, false},
        {"ancientdecay", 3.2f, 0.16f, 0.08f, 20.0f, 0.4f, 0, false},
        {"ancientair", 0.42f, 0.42f, 0.0f, 1.0f, 1.0f, 0, false},
        {"ancientdetune", 0.32f, 0.32f, 0.0f, 1.0f, 1.0f, 0, false}};
    const auto ancientRecipe = pas1::generation::SoundGenerator::createRecipe(
        ancientBase, "Aulos reed drone", controls);
    expect(std::abs(findValue(ancientRecipe, "ancientinstrument")->normalizedValue - 0.25f)
               < 0.001f,
           "aulos recipe must select the paired-reed instrument");
    expect(findValue(ancientRecipe, "ancientmix")->normalizedValue > 0.8f,
           "ancient instrument recipes must bring the modeled voice forward");
    const auto lyreRecipe = pas1::generation::SoundGenerator::createRecipe(
        ancientBase, "ancient lyre", controls);
    expect(std::abs(findValue(lyreRecipe, "ancientinstrument")->normalizedValue - 0.5f)
               < 0.001f,
           "lyre keywords must select the plucked-string voice");
    const auto uppercaseAulosRecipe = pas1::generation::SoundGenerator::createRecipe(
        ancientBase, "АУЛОС", controls);
    expect(std::abs(findValue(uppercaseAulosRecipe, "ancientinstrument")->normalizedValue - 0.25f)
               < 0.001f,
           "uppercase Russian instrument names must be recognized");
    const auto bronzeGongRecipe = pas1::generation::SoundGenerator::createRecipe(
        ancientBase, "БРОНЗОВЫЙ ГОНГ", controls);
    expect(std::abs(findValue(bronzeGongRecipe, "ancientinstrument")->normalizedValue - 0.75f)
               < 0.001f,
           "uppercase Russian gong descriptions must select the bronze gong");
    const auto russianWarmBassRecipe = pas1::generation::SoundGenerator::createRecipe(
        base, "ТЁПЛЫЙ БАС", controls);
    expect(findValue(russianWarmBassRecipe, "sublevel")->normalizedValue > 0.0f,
           "uppercase Russian warm-bass prompts must affect bass parameters");
    const auto englishBassVariation = pas1::generation::SoundGenerator::createVariation(
        base, pas1::generation::VariationMode::genre, 0.8f, 24680u, {}, "bass");
    const auto russianBassVariation = pas1::generation::SoundGenerator::createVariation(
        base, pas1::generation::VariationMode::genre, 0.8f, 24680u, {}, "БАС");
    expect(std::abs(findValue(englishBassVariation, "sublevel")->normalizedValue
                    - findValue(russianBassVariation, "sublevel")->normalizedValue)
               < 0.001f,
           "Russian genre hints must work like their English equivalents");
    const std::vector<std::string> lockedTine{"musicboxmix"};
    const auto lockedMusicBoxRecipe = pas1::generation::SoundGenerator::createRecipe(
        musicBoxBase, "music box", controls, lockedTine);
    expect(findMusicBoxValue(lockedMusicBoxRecipe, "musicboxmix")->normalizedValue
               == findMusicBoxValue(musicBoxBase, "musicboxmix")->normalizedValue,
           "recipe generation must respect locked parameters");
    const auto lockedTineVariation = pas1::generation::SoundGenerator::createVariation(
        musicBoxBase, pas1::generation::VariationMode::seeded, 0.85f, 24680u, lockedTine);
    expect(findMusicBoxValue(lockedTineVariation, "musicboxmix")->normalizedValue
               == findMusicBoxValue(musicBoxBase, "musicboxmix")->normalizedValue,
           "variation generation must respect locked music-box controls");
    expect(pas1::generation::SoundGenerator::belongsToSection(
               "musicboxdecay", pas1::generation::VariationSection::oscillators),
           "music-box tine controls must belong to the oscillator lock section");
    expect(pas1::generation::SoundGenerator::belongsToSection(
               "ancientmix", pas1::generation::VariationSection::oscillators),
           "ancient-instrument controls must belong to the oscillator lock section");
    expect(pas1::generation::SoundGenerator::belongsToSection(
               "filtercutoff", pas1::generation::VariationSection::filterAndEnvelope),
           "filter controls must belong to the filter lock section");
    expect(pas1::generation::SoundGenerator::belongsToSection(
               "lfo2rate", pas1::generation::VariationSection::modulation),
           "LFO controls must belong to the modulation lock section");
    expect(pas1::generation::SoundGenerator::belongsToSection(
               "reverbmix", pas1::generation::VariationSection::effects),
           "reverb controls must belong to the effects lock section");

    const std::vector<std::string> locks{"filtercutoff", "master"};
    const auto first = pas1::generation::SoundGenerator::createVariation(
        recipe, pas1::generation::VariationMode::seeded, 0.72f, 13579u, locks, "bass");
    const auto repeated = pas1::generation::SoundGenerator::createVariation(
        recipe, pas1::generation::VariationMode::seeded, 0.72f, 13579u, locks, "bass");
    const auto zeroAmount = pas1::generation::SoundGenerator::createVariation(
        recipe, pas1::generation::VariationMode::aggressive, 0.0f, 42u, {}, "lead");
    expect(first.size() == recipe.size(), "variation must preserve the parameter list");
    expect(findValue(first, "filtercutoff")->normalizedValue
               == findValue(recipe, "filtercutoff")->normalizedValue,
           "locked parameters must remain unchanged");
    expect(findValue(first, "master")->normalizedValue == findValue(recipe, "master")->normalizedValue,
           "master output must not be randomized");
    expect(findValue(first, "waveform")->normalizedValue
               == findValue(repeated, "waveform")->normalizedValue,
           "the same seed must reproduce a variation");
    for (const auto& parameter : first)
        expect(parameter.normalizedValue >= 0.0f && parameter.normalizedValue <= 1.0f,
               "variation values must remain in the normalized range");
    for (std::size_t index = 0; index < recipe.size(); ++index)
        expect(zeroAmount[index].normalizedValue == recipe[index].normalizedValue,
               "zero variation amount must preserve the current sound");
}

void testSampleRateAndBlockStress() {
    constexpr std::array<double, 5> sampleRates{8000.0, 44100.0, 48000.0, 96000.0, 192000.0};
    constexpr std::array<int, 5> blockSizes{1, 16, 64, 257, 1024};
    pas1::dsp::WavetableOscillator::initialize();

    for (const auto sampleRate : sampleRates) {
        for (const auto blockSize : blockSizes) {
            pas1::dsp::WavetableOscillator oscillator;
            pas1::dsp::StateVariableFilter filter;
            pas1::dsp::AmpEnvelope envelope;
            filter.prepare(sampleRate);
            filter.setParameters(static_cast<float>(sampleRate * 0.1), 0.7f,
                                 pas1::dsp::FilterType::lowPass);
            envelope.setSampleRate(sampleRate);
            envelope.setParameters({0.003f, 0.04f, 0.6f, 0.08f});
            envelope.noteOn();

            for (int block = 0; block < 4; ++block) {
                for (int sampleIndex = 0; sampleIndex < blockSize; ++sampleIndex) {
                    const auto sample = oscillator.nextSample(
                        440.0f, static_cast<float>(sampleRate), pas1::dsp::Waveform::saw);
                    const auto output = filter.processSample(sample) * envelope.nextSample();
                    expect(std::isfinite(output),
                           "oscillator, filter, and envelope must remain finite at supported rates and block sizes");
                }
            }
        }
    }
}
} // namespace

int main() {
    testOscillator();
    testMetallicTine();
    testAncientInstrumentVoices();
    testEnvelope();
    testWavetableOscillator();
    testFilter();
    testLfo();
    testEffects();
    testModulationMatrix();
    testPortamento();
    testNotePriorityStack();
    testMorphMatrix();
    testSoundDNA();
    testSoundGenerator();
    testSampleRateAndBlockStress();
    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "PAS 1 core tests passed\n";
    return EXIT_SUCCESS;
}
