#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/AmpEnvelope.h"
#include "DSP/EffectsChain.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/MorphMatrix.h"
#include "DSP/Oscillator.h"
#include "Presets/PresetManager.h"

#include <atomic>
#include <array>
#include <cstdint>
#include <memory>

class Pas1AudioProcessor final : public juce::AudioProcessor,
                                 private juce::MidiKeyboardState::Listener {
public:
    Pas1AudioProcessor();
    ~Pas1AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void captureMorphCorner(int corner);
    void captureFactoryMorphCorners();
    void randomizeMorphCorners(std::uint32_t seed);
    void setMorphParameterLocked(int parameter, bool locked);
    bool isMorphParameterLocked(int parameter) const;
    bool isMorphConfigured() const noexcept;
    static juce::String getMorphParameterName(int parameter);
    void copyRecentAudioSamples(float* destination, int numSamples) const noexcept;

    juce::AudioProcessorValueTreeState parameters;
    PresetManager presetManager;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> hostTempoBpm{120.0f};
    std::atomic<float> outputPeak{0.0f};
    std::atomic<float> midiActivity{0.0f};
    std::atomic<float> visualSampleRate{44100.0f};
    pas1::dsp::EffectsChain effectsChain;

private:
    struct KeyboardEvent {
        int channel = 1;
        int note = 60;
        float velocity = 0.0f;
        bool isNoteOn = false;
    };
    static constexpr std::uint32_t keyboardEventCapacity = 256;
    void handleNoteOn(juce::MidiKeyboardState*, int channel, int note, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int channel, int note, float velocity) override;
    void enqueueKeyboardEvent(KeyboardEvent event) noexcept;
    bool dequeueKeyboardEvent(KeyboardEvent& event) noexcept;

    static constexpr std::size_t spectrumCaptureSize = 4096;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateMorphValues(int blockSize) noexcept;
    void writeMorphState(juce::ValueTree& state) const;
    void readMorphState(const juce::ValueTree& state);

    std::unique_ptr<juce::Synthesiser> synthesiser;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterGain;
    pas1::dsp::MorphMatrix morphMatrix;
    std::array<std::atomic<float>, pas1::dsp::MorphMatrix::parameterCount> morphValues{};
    std::array<std::atomic<float>, spectrumCaptureSize> spectrumCapture{};
    std::atomic<std::uint64_t> spectrumWriteIndex{0};
    std::array<KeyboardEvent, keyboardEventCapacity> keyboardEvents{};
    std::atomic<std::uint32_t> keyboardEventWriteIndex{0};
    std::atomic<std::uint32_t> keyboardEventReadIndex{0};
    std::atomic<bool> keyboardEventOverflow{false};
    pas1::dsp::ModulationAccumulator modulationAccumulator;
    float modulationMorphX = 0.0f;
    float modulationMorphY = 0.0f;
    double morphSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pas1AudioProcessor)
};
