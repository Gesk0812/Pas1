#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "DSP/LowFrequencyOscillator.h"
#include "DSP/MacroControls.h"
#include "DSP/MetallicTine.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/NotePriorityStack.h"
#include "DSP/Portamento.h"
#include "DSP/StateVariableFilter.h"
#include "DSP/Tuning.h"
#include "DSP/WavetableOscillator.h"
#include "UI/Utf8Text.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace {

constexpr int kMaximumVoices = 32;
constexpr double kTwoPi = 6.28318530717958647692;

template <typename BaseParameter>
class Utf8NamedParameter final : public BaseParameter {
public:
    template <typename... Args>
    Utf8NamedParameter(juce::ParameterID id, const char* name, Args&&... args)
        : BaseParameter(std::move(id), pas1::ui::utf8String(name),
                        std::forward<Args>(args)...) {}

    template <typename... Args>
    Utf8NamedParameter(juce::ParameterID id, juce::String name, Args&&... args)
        : BaseParameter(std::move(id), std::move(name),
                        std::forward<Args>(args)...) {}
};

constexpr std::array<const char*, pas1::dsp::ModulationMatrix::sourceCount>
    kModulationSourceNames{
        "НЧ-генератор 1", "НЧ-генератор 2", "НЧ-генератор 3", "НЧ-генератор 4",
        "Амплитудная огибающая", "Огибающая фильтра", "Скорость нажатия",
        "Колесо модуляции", "Изгиб высоты", "Отслеживание клавиши", "Случайно для ноты"};
constexpr std::array<const char*, pas1::dsp::ModulationMatrix::destinationCount>
    kModulationDestinationNames{
        "Высота тона", "Уровень осц. 1", "Позиция волновой таблицы", "Срез фильтра",
        "Резонанс", "Глубина ЧМ", "Панорама", "Уровень искажения",
        "Уровень реверберации", "Морфинг X", "Морфинг Y"};
constexpr std::array<const char*, pas1::dsp::MorphMatrix::parameterCount> kMorphParameterIds{
    "osc1level", "osc2level", "osc3level", "wavetablescan", "unisondetune",
    "filtercutoff", "filterresonance", "attack", "release", "lfo1amount",
    "fmamount", "filterdrive", "distortionmix", "stereowidth", "reverbmix"};

struct LfoParameterPointers {
    std::atomic<float>* rate = nullptr;
    std::atomic<float>* amount = nullptr;
    std::atomic<float>* waveform = nullptr;
    std::atomic<float>* destination = nullptr;
    std::atomic<float>* sync = nullptr;
    std::atomic<float>* division = nullptr;
    std::atomic<float>* phase = nullptr;
    std::atomic<float>* fadeIn = nullptr;
    std::atomic<float>* retrigger = nullptr;
};

struct ModulationRouteParameterPointers {
    std::atomic<float>* source = nullptr;
    std::atomic<float>* destination = nullptr;
    std::atomic<float>* amount = nullptr;
    std::atomic<float>* polarity = nullptr;
    std::atomic<float>* enabled = nullptr;
};

struct EngineParameterPointers {
    std::atomic<float>* oscillator1Level = nullptr;
    std::atomic<float>* oscillator1Octave = nullptr;
    std::atomic<float>* oscillator1Fine = nullptr;
    std::atomic<float>* oscillator1Phase = nullptr;
    std::atomic<float>* wavetableScan = nullptr;
    std::atomic<float>* unisonVoices = nullptr;
    std::atomic<float>* unisonDetune = nullptr;
    std::atomic<float>* oscillator2Waveform = nullptr;
    std::atomic<float>* oscillator2Level = nullptr;
    std::atomic<float>* oscillator2Detune = nullptr;
    std::atomic<float>* oscillator2Phase = nullptr;
    std::atomic<float>* oscillator2HardSync = nullptr;
    std::atomic<float>* oscillator2FmAmount = nullptr;
    std::atomic<float>* oscillator2RingAmount = nullptr;
    std::atomic<float>* oscillator3Waveform = nullptr;
    std::atomic<float>* oscillator3Level = nullptr;
    std::atomic<float>* pulseWidth = nullptr;
    std::atomic<float>* subLevel = nullptr;
    std::atomic<float>* noiseLevel = nullptr;
    std::atomic<float>* musicBoxMix = nullptr;
    std::atomic<float>* musicBoxBrightness = nullptr;
    std::atomic<float>* musicBoxDecay = nullptr;
    std::atomic<float>* musicBoxResonance = nullptr;
    std::atomic<float>* musicBoxInharmonicity = nullptr;
    std::atomic<float>* musicBoxStrike = nullptr;
    std::atomic<float>* ancientInstrument = nullptr;
    std::atomic<float>* ancientMix = nullptr;
    std::atomic<float>* ancientBrightness = nullptr;
    std::atomic<float>* ancientDecay = nullptr;
    std::atomic<float>* ancientAir = nullptr;
    std::atomic<float>* ancientPipeDetune = nullptr;
    std::atomic<float>* macroBody = nullptr;
    std::atomic<float>* macroBrightness = nullptr;
    std::atomic<float>* macroMovement = nullptr;
    std::atomic<float>* tuningOctave = nullptr;
    std::atomic<float>* transpose = nullptr;
    std::atomic<float>* masterFineTune = nullptr;
    std::atomic<float>* temperament = nullptr;
    std::atomic<float>* tuningRoot = nullptr;
    std::atomic<float>* filterType = nullptr;
    std::atomic<float>* filterCutoff = nullptr;
    std::atomic<float>* filterResonance = nullptr;
    std::atomic<float>* filterDrive = nullptr;
    std::atomic<float>* filterEnvelopeAmount = nullptr;
    std::atomic<float>* filterKeyTracking = nullptr;
    std::atomic<float>* filterVelocityAmount = nullptr;
    std::atomic<float>* filterModWheelAmount = nullptr;
    std::atomic<float>* filterRandomAmount = nullptr;
    std::atomic<float>* filterAttack = nullptr;
    std::atomic<float>* filterDecay = nullptr;
    std::atomic<float>* filterSustain = nullptr;
    std::atomic<float>* filterRelease = nullptr;
    std::atomic<float>* hostTempoBpm = nullptr;
    std::array<LfoParameterPointers, 4> lfos{};
    std::array<ModulationRouteParameterPointers,
               pas1::dsp::ModulationMatrix::routeCount> modulationRoutes{};
};

class Pas1Sound final : public juce::SynthesiserSound {
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Pas1Voice final : public juce::SynthesiserVoice {
public:
    Pas1Voice(std::atomic<float>* waveform,
              std::atomic<float>* attack,
              std::atomic<float>* decay,
              std::atomic<float>* sustain,
              std::atomic<float>* release,
              std::atomic<float>* polyphony,
              std::atomic<float>* voiceMode,
              std::atomic<float>* glideEnabled,
              std::atomic<float>* glideTime,
              EngineParameterPointers engineParameters,
               pas1::dsp::ModulationAccumulator* modulationAccumulator,
              int voiceIndex)
        : waveform_(waveform),
          attack_(attack),
          decay_(decay),
          sustain_(sustain),
          release_(release),
          polyphony_(polyphony),
          voiceMode_(voiceMode),
          glideEnabled_(glideEnabled),
          glideTime_(glideTime),
          engineParameters_(engineParameters),
          modulationAccumulator_(modulationAccumulator),
          voiceIndex_(voiceIndex) {}

    bool canPlaySound(juce::SynthesiserSound* sound) override {
        const auto mode = static_cast<int>(std::round(voiceMode_->load()));
        const auto hasVoiceSlot = mode == 0
                                      ? voiceIndex_ < static_cast<int>(std::round(polyphony_->load()))
                                      : voiceIndex_ == 0;
        return dynamic_cast<Pas1Sound*>(sound) != nullptr && hasVoiceSlot;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override {
        const auto mode = static_cast<int>(std::round(voiceMode_->load()));
        const auto legatoTransition = mode == 2 && legatoForNextNote_;
        const auto wasMonoTransition = monoStealPending_;
        monoStealPending_ = false;
        legatoForNextNote_ = false;
        const auto tunedFrequency = pas1::dsp::tunedMidiFrequency(
            midiNoteNumber,
            static_cast<int>(std::round(engineParameters_.tuningOctave->load())),
            static_cast<int>(std::round(engineParameters_.transpose->load())),
            engineParameters_.masterFineTune->load(),
            static_cast<int>(std::round(engineParameters_.temperament->load())),
            static_cast<int>(std::round(engineParameters_.tuningRoot->load())));
        const auto targetFrequency = std::clamp(
            tunedFrequency, 1.0f, static_cast<float>(std::max(20.0, currentSampleRate_ * 0.45)));
        const auto glideSeconds = glideTime_->load();
        if (mode != 0 && wasMonoTransition && glideEnabled_->load() >= 0.5f
            && std::isfinite(glideSeconds) && glideSeconds > 0.0f) {
            const auto duration = static_cast<std::uint64_t>(std::llround(
                std::clamp(static_cast<double>(glideSeconds) * currentSampleRate_,
                           1.0, 1000000000.0)));
            portamento_.glideTo(targetFrequency, duration);
        } else {
            portamento_.reset(targetFrequency);
        }
        velocity_ = velocity;
        midiNoteNumber_ = midiNoteNumber;
        if (!legatoTransition) {
            vibratoPhase_ = 0.0;
            const auto oscillator1Phase = engineParameters_.oscillator1Phase->load();
            for (auto& unisonOscillator : unisonOscillators_)
                unisonOscillator.reset(oscillator1Phase);
            oscillator2_.reset(engineParameters_.oscillator2Phase->load());
            oscillator3_.reset();
            subOscillator_.reset();
            filter_.reset();
            filterParametersInitialized_ = false;
            filterEnvelope_.setParameters(readFilterEnvelopeParameters());
            filterEnvelope_.noteOn();
            noteAgeSamples_ = 0;
            for (std::size_t index = 0; index < lfos_.size(); ++index) {
                const auto& lfo = engineParameters_.lfos[index];
                if (lfo.retrigger->load() >= 0.5f)
                    lfos_[index].reset(
                        static_cast<std::uint32_t>(midiNoteNumber * 97 + voiceIndex_ * 7919
                                                   + static_cast<int>(index) * 104729),
                        lfo.phase->load());
            }
        }
        noiseState_ = static_cast<std::uint32_t>(midiNoteNumber * 131 + voiceIndex_ * 977 + 1);
        if (noiseState_ == 0)
            noiseState_ = 0x6d2b79f5u;
        randomPerNote_ = nextNoise();
        musicBoxTine_.trigger(
            targetFrequency, static_cast<float>(currentSampleRate_),
            engineParameters_.musicBoxBrightness->load(),
            engineParameters_.musicBoxDecay->load(),
            engineParameters_.musicBoxResonance->load(),
            engineParameters_.musicBoxInharmonicity->load(),
            engineParameters_.musicBoxStrike->load());
        ancientInstrument_.trigger(
            static_cast<pas1::dsp::AncientInstrument>(std::clamp(
                static_cast<int>(std::round(engineParameters_.ancientInstrument->load())),
                0, 4)),
            targetFrequency, static_cast<float>(currentSampleRate_),
            engineParameters_.ancientBrightness->load(),
            engineParameters_.ancientDecay->load(),
            engineParameters_.ancientAir->load(),
            engineParameters_.ancientPipeDetune->load(),
            static_cast<std::uint32_t>(midiNoteNumber * 131 + voiceIndex_ * 977 + 1));
        updateEnvelopeParameters();
        if (!legatoTransition)
            envelope_.noteOn();
    }

    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) {
            monoStealPending_ = false;
            updateEnvelopeParameters();
            envelope_.noteOff();
            filterEnvelope_.noteOff();
        } else {
            const auto mode = static_cast<int>(std::round(voiceMode_->load()));
            monoStealPending_ = !modeResetInProgress_ && mode != 0 && isVoiceActive();
            clearCurrentNote();
            if (monoStealPending_)
                return;
            envelope_.noteOff();
            filterEnvelope_.noteOff();
        }
    }

    void setLegatoForNextNote(bool isLegato) noexcept { legatoForNextNote_ = isLegato; }
    void setModeResetInProgress(bool isResetting) noexcept { modeResetInProgress_ = isResetting; }

    void pitchWheelMoved(int newValue) override {
        const auto normalized = (static_cast<float>(newValue) - 8192.0f) / 8192.0f;
        pitchBendNormalized_ = std::clamp(normalized, -1.0f, 1.0f);
        pitchBendRatio_ = std::pow(2.0f, normalized * (2.0f / 12.0f));
    }

    void controllerMoved(int controllerNumber, int newValue) override {
        if (controllerNumber == 1)
            modWheel_ = static_cast<float>(newValue) / 127.0f;
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
        if (!isVoiceActive())
            return;

        updateEnvelopeParameters();
        const auto selectedWave = static_cast<int>(std::round(waveform_->load()));
        const auto waveform = static_cast<pas1::dsp::Waveform>(
            std::clamp(selectedWave, 0, static_cast<int>(pas1::dsp::Waveform::count) - 1));
        const auto osc2Wave = static_cast<pas1::dsp::Waveform>(
            std::clamp(static_cast<int>(std::round(engineParameters_.oscillator2Waveform->load())),
                       0, static_cast<int>(pas1::dsp::Waveform::count) - 1));
        const auto osc3Wave = static_cast<pas1::dsp::Waveform>(
            std::clamp(static_cast<int>(std::round(engineParameters_.oscillator3Waveform->load())),
                       0, static_cast<int>(pas1::dsp::Waveform::count) - 1));
        const auto filterType = static_cast<pas1::dsp::FilterType>(
            std::clamp(static_cast<int>(std::round(engineParameters_.filterType->load())), 0, 3));
        const auto baseCutoff = engineParameters_.filterCutoff->load();
        const auto resonance = engineParameters_.filterResonance->load();
        const auto filterDrive = engineParameters_.filterDrive->load();
        const auto filterEnvelopeAmount = engineParameters_.filterEnvelopeAmount->load();
        const auto filterKeyTracking = engineParameters_.filterKeyTracking->load();
        const auto filterVelocityAmount = engineParameters_.filterVelocityAmount->load();
        const auto filterModWheelAmount = engineParameters_.filterModWheelAmount->load();
        const auto filterRandomAmount = engineParameters_.filterRandomAmount->load();
        const auto osc1Level = engineParameters_.oscillator1Level->load();
        const auto oscillator1PitchRatio = std::pow(
            2.0f, engineParameters_.oscillator1Octave->load()
                      + engineParameters_.oscillator1Fine->load() / 100.0f);
        const auto macroBody = pas1::dsp::macro::normalizePercent(
            engineParameters_.macroBody->load());
        const auto macroBrightness = pas1::dsp::macro::normalizePercent(
            engineParameters_.macroBrightness->load());
        const auto macroMovementScale = pas1::dsp::macro::movementScale(
            pas1::dsp::macro::normalizePercent(engineParameters_.macroMovement->load()));
        const auto wavetableScan = std::clamp(
            engineParameters_.wavetableScan->load()
                + pas1::dsp::macro::brightnessWavetableOffset(macroBrightness),
            0.0f, 1.0f);
        const auto unisonVoices = std::clamp(
            static_cast<int>(std::round(engineParameters_.unisonVoices->load())), 1, 7);
        const auto osc2BaseLevel = pas1::dsp::macro::bodyOscillatorLevel(
            engineParameters_.oscillator2Level->load(), macroBody);
        const auto osc3Level = engineParameters_.oscillator3Level->load();
        const auto pulseWidth = engineParameters_.pulseWidth->load();
        const auto subLevel = pas1::dsp::macro::bodySubLevel(
            engineParameters_.subLevel->load(), macroBody);
        const auto effectiveUnisonDetune = std::clamp(
            engineParameters_.unisonDetune->load() * macroMovementScale, 0.0f, 100.0f);
        const auto noiseLevel = engineParameters_.noiseLevel->load();
        const auto musicBoxMix = std::clamp(engineParameters_.musicBoxMix->load(), 0.0f, 1.0f);
        const auto ancientMode = std::clamp(
            static_cast<int>(std::round(engineParameters_.ancientInstrument->load())), 0, 4);
        const auto ancientMix = ancientMode == 0
                                    ? 0.0f
                                    : std::clamp(engineParameters_.ancientMix->load(), 0.0f, 1.0f);
        const auto detuneRatio = std::pow(2.0f, engineParameters_.oscillator2Detune->load() / 1200.0f);
        const auto hardSync = engineParameters_.oscillator2HardSync->load() >= 0.5f;
        const auto fmAmount = engineParameters_.oscillator2FmAmount->load();
        const auto ringAmount = engineParameters_.oscillator2RingAmount->load();
        filterEnvelope_.setParameters(readFilterEnvelopeParameters());

        struct LfoSettings {
            float rate;
            float amount;
            int waveform;
            int destination;
            bool sync;
            int division;
            float phase;
            float fadeIn;
            bool retrigger;
        };
        std::array<LfoSettings, 4> lfoSettings{};
        const auto hostBpm = engineParameters_.hostTempoBpm->load();
        for (std::size_t index = 0; index < lfoSettings.size(); ++index) {
            const auto& source = engineParameters_.lfos[index];
            lfoSettings[index] = {
                source.rate->load(),
                std::clamp(source.amount->load() * macroMovementScale, 0.0f, 1.0f),
                std::clamp(static_cast<int>(std::round(source.waveform->load())), 0, 5),
                std::clamp(static_cast<int>(std::round(source.destination->load())), 0, 4),
                source.sync->load() >= 0.5f,
                std::clamp(static_cast<int>(std::round(source.division->load())), 0, 4),
                source.phase->load(),
                source.fadeIn->load(),
                source.retrigger->load() >= 0.5f
            };
        }
        pas1::dsp::ModulationMatrix::Routes modulationRoutes{};
        for (std::size_t index = 0; index < modulationRoutes.size(); ++index) {
            const auto& routeParameters = engineParameters_.modulationRoutes[index];
            auto& route = modulationRoutes[index];
            route.source = static_cast<pas1::dsp::ModulationSource>(std::clamp(
                static_cast<int>(std::round(routeParameters.source->load())),
                0, static_cast<int>(pas1::dsp::ModulationMatrix::sourceCount) - 1));
            route.destination = static_cast<pas1::dsp::ModulationDestination>(std::clamp(
                static_cast<int>(std::round(routeParameters.destination->load())),
                0, static_cast<int>(pas1::dsp::ModulationMatrix::destinationCount) - 1));
            route.amount = std::clamp(
                routeParameters.amount->load() * macroMovementScale, -1.0f, 1.0f);
            route.polarity = routeParameters.polarity->load() >= 0.5f
                                 ? pas1::dsp::ModulationPolarity::unipolar
                                 : pas1::dsp::ModulationPolarity::bipolar;
            route.enabled = routeParameters.enabled->load() >= 0.5f;
        }
        constexpr std::array<float, 5> quarterNotesPerCycle{4.0f, 2.0f, 1.0f, 0.5f, 0.25f};

        const auto sampleRate = getSampleRate();

        for (int sample = 0; sample < numSamples; ++sample) {
            auto pitchModulation = std::sin(vibratoPhase_) * modWheel_ * 0.5f;
            auto cutoffOctaves = pas1::dsp::macro::bodyFilterOctaves(macroBody)
                                 + pas1::dsp::macro::brightnessFilterOctaves(macroBrightness);
            auto oscillator2Level = osc2BaseLevel;
            auto amplitudeLfoGain = 1.0f;
            pas1::dsp::ModulationMatrix::SourceValues modulationSources{};
            const auto amplitudeEnvelopeValue = envelope_.nextSample();
            const auto filterEnvelopeValue = filterEnvelope_.nextSample();
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::ampEnvelope)] = amplitudeEnvelopeValue * 2.0f - 1.0f;
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::filterEnvelope)] = filterEnvelopeValue * 2.0f - 1.0f;
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::velocity)] = velocity_ * 2.0f - 1.0f;
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::modWheel)] = modWheel_ * 2.0f - 1.0f;
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::pitchBend)] = pitchBendNormalized_;
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::keyTracking)] = std::clamp(
                    (static_cast<float>(midiNoteNumber_) - 60.0f) / 48.0f, -1.0f, 1.0f);
            modulationSources[static_cast<std::size_t>(
                pas1::dsp::ModulationSource::randomPerNote)] = randomPerNote_;
            for (std::size_t index = 0; index < lfos_.size(); ++index) {
                const auto lfoRate = lfoSettings[index].sync
                                         ? hostBpm / (60.0f * quarterNotesPerCycle[
                                               static_cast<std::size_t>(lfoSettings[index].division)])
                                         : lfoSettings[index].rate;
                const auto lfoValue = lfos_[index].nextSample(
                    lfoRate, static_cast<float>(sampleRate),
                    static_cast<pas1::dsp::LfoWaveform>(lfoSettings[index].waveform));
                const auto fadeSamples = lfoSettings[index].fadeIn * static_cast<float>(sampleRate);
                const auto fadeGain = fadeSamples > 0.0f
                                          ? std::clamp(static_cast<float>(noteAgeSamples_) / fadeSamples,
                                                       0.0f, 1.0f)
                                          : 1.0f;
                const auto modulation = lfoValue * lfoSettings[index].amount * fadeGain;
                modulationSources[index] = lfoValue * fadeGain;
                switch (lfoSettings[index].destination) {
                    case 1:
                        pitchModulation += modulation * 2.0f;
                        break;
                    case 2:
                        cutoffOctaves += modulation * 4.0f;
                        break;
                    case 3:
                        oscillator2Level += modulation;
                        break;
                    case 4:
                        amplitudeLfoGain = std::clamp(1.0f + modulation, 0.0f, 2.0f);
                        break;
                    default:
                        break;
                }
            }
            const auto modulation = pas1::dsp::ModulationMatrix::evaluate(
                modulationSources, modulationRoutes);
            const auto modulated = [&modulation](pas1::dsp::ModulationDestination destination) {
                return modulation[static_cast<std::size_t>(destination)];
            };
            pitchModulation += modulated(pas1::dsp::ModulationDestination::pitch) * 12.0f;
            cutoffOctaves +=
                modulated(pas1::dsp::ModulationDestination::filterCutoff) * 4.0f;
            const auto modulatedOscillator1Level = std::clamp(
                osc1Level + modulated(pas1::dsp::ModulationDestination::oscillator1Level),
                0.0f, 1.0f);
            const auto modulatedWavetableScan = std::clamp(
                wavetableScan
                    + modulated(pas1::dsp::ModulationDestination::wavetablePosition) * 0.5f,
                0.0f, 1.0f);
            const auto modulatedResonance = std::clamp(
                resonance + modulated(pas1::dsp::ModulationDestination::resonance) * 0.5f,
                0.0f, 1.0f);
            const auto modulatedFmAmount = std::clamp(
                fmAmount + modulated(pas1::dsp::ModulationDestination::fmAmount) * 0.5f,
                0.0f, 1.0f);
            const auto pan = std::clamp(modulated(pas1::dsp::ModulationDestination::pan),
                                        -1.0f, 1.0f);
            modulationAccumulator_->add(modulation);

            const auto frequency = portamento_.nextSample() * pitchBendRatio_
                                   * std::pow(2.0f, pitchModulation / 12.0f);
            const auto oscillator1Frequency = frequency * oscillator1PitchRatio;
            const auto masterPhaseBefore = unisonOscillators_[0].phase();
            float oscillator1Sample = 0.0f;
            for (int unisonIndex = 0; unisonIndex < unisonVoices; ++unisonIndex) {
                const auto detunePosition = unisonVoices == 1
                                                ? 0.0f
                                                : (2.0f * static_cast<float>(unisonIndex)
                                                   / static_cast<float>(unisonVoices - 1))
                                                      - 1.0f;
                const auto unisonFrequency = oscillator1Frequency
                                              * std::pow(2.0f, detunePosition
                                                                 * effectiveUnisonDetune
                                                                 / 1200.0f);
                oscillator1Sample += unisonOscillators_[static_cast<std::size_t>(unisonIndex)].nextSample(
                    unisonFrequency, static_cast<float>(sampleRate), waveform, modulatedWavetableScan);
            }
            oscillator1Sample /= std::sqrt(static_cast<float>(unisonVoices));
            if (hardSync && unisonOscillators_[0].phase() < masterPhaseBefore)
                oscillator2_.reset();
            const auto frequency2 = frequency * detuneRatio
                                    * std::pow(2.0f, oscillator1Sample * modulatedFmAmount);
            auto oscillator2Sample = oscillator2_.nextSample(
                frequency2, static_cast<float>(sampleRate), osc2Wave);
            oscillator2Sample = oscillator2Sample * (1.0f - ringAmount)
                                + oscillator2Sample * oscillator1Sample * ringAmount;
            cutoffOctaves += filterEnvelopeValue * filterEnvelopeAmount * 4.0f;
            cutoffOctaves += (static_cast<float>(midiNoteNumber_) - 60.0f) / 12.0f
                             * filterKeyTracking;
            cutoffOctaves += (velocity_ - 0.5f) * filterVelocityAmount * 4.0f;
            cutoffOctaves += modWheel_ * filterModWheelAmount * 4.0f;
            cutoffOctaves += randomPerNote_ * filterRandomAmount * 4.0f;
            if ((sample & 7) == 0) {
                const auto safeOctaves = std::clamp(cutoffOctaves, -12.0f, 12.0f);
                auto targetCutoff = baseCutoff * std::exp2(safeOctaves);
                if (!std::isfinite(targetCutoff) || targetCutoff <= 0.0f)
                    targetCutoff = baseCutoff;
                targetCutoff = std::clamp(
                    targetCutoff, 10.0f,
                    static_cast<float>(std::min(20000.0, currentSampleRate_ * 0.45)));
                const auto targetLogCutoff = std::log2(targetCutoff);
                const auto targetResonance = std::clamp(modulatedResonance, 0.0f, 1.0f);
                if (!filterParametersInitialized_) {
                    smoothedFilterLogCutoff_ = targetLogCutoff;
                    smoothedFilterResonance_ = targetResonance;
                    filterParametersInitialized_ = true;
                } else {
                    constexpr double smoothingTimeSeconds = 0.015;
                    const auto smoothing = static_cast<float>(
                        1.0 - std::exp(-8.0 / (currentSampleRate_ * smoothingTimeSeconds)));
                    smoothedFilterLogCutoff_ +=
                        smoothing * (targetLogCutoff - smoothedFilterLogCutoff_);
                    smoothedFilterResonance_ +=
                        smoothing * (targetResonance - smoothedFilterResonance_);
                }
                filter_.setParameters(std::exp2(smoothedFilterLogCutoff_),
                                      smoothedFilterResonance_, filterType);
            }

            auto signal = oscillator1Sample * modulatedOscillator1Level
                          + oscillator2Sample * std::clamp(oscillator2Level, 0.0f, 1.0f)
                          + oscillator3_.nextSample(frequency, static_cast<float>(sampleRate), osc3Wave,
                                                    pulseWidth)
                                * std::clamp(osc3Level, 0.0f, 1.0f)
                          + subOscillator_.nextSample(frequency * 0.5f, static_cast<float>(sampleRate),
                                                      pas1::dsp::Waveform::square)
                                * std::clamp(subLevel, 0.0f, 1.0f)
                          + nextNoise() * std::clamp(noiseLevel, 0.0f, 1.0f);
            const auto levelSum = modulatedOscillator1Level
                                  + std::clamp(oscillator2Level, 0.0f, 1.0f)
                                  + std::clamp(osc3Level, 0.0f, 1.0f)
                                  + std::clamp(subLevel, 0.0f, 1.0f)
                                  + std::clamp(noiseLevel, 0.0f, 1.0f);
            if (levelSum > 0.0f)
                signal /= levelSum;
            if (musicBoxMix > 0.0f) {
                const auto tineSample = musicBoxTine_.nextSample(
                    frequency, static_cast<float>(sampleRate));
                signal = signal * (1.0f - musicBoxMix) + tineSample * musicBoxMix;
            }
            if (ancientMix > 0.0f) {
                const auto ancientSample = ancientInstrument_.nextSample(
                    frequency, static_cast<float>(sampleRate));
                signal = signal * (1.0f - ancientMix) + ancientSample * ancientMix;
            }
            if (filterDrive > 0.0f) {
                const auto drive = 1.0f + std::clamp(filterDrive, 0.0f, 1.0f) * 9.0f;
                signal = std::tanh(signal * drive) / std::tanh(drive);
            }
            auto value = filter_.processSample(signal) * amplitudeEnvelopeValue * velocity_
                         * amplitudeLfoGain;
            if (!std::isfinite(value))
                value = 0.0f;

            vibratoPhase_ += 6.28318530717958647692 * 5.0 / sampleRate;
            if (vibratoPhase_ >= kTwoPi)
                vibratoPhase_ -= kTwoPi;

            if (outputBuffer.getNumChannels() == 1) {
                outputBuffer.addSample(0, startSample + sample, value);
            } else {
                outputBuffer.addSample(0, startSample + sample,
                                       value * (pan > 0.0f ? 1.0f - pan : 1.0f));
                outputBuffer.addSample(1, startSample + sample,
                                       value * (pan < 0.0f ? 1.0f + pan : 1.0f));
            }
            ++noteAgeSamples_;
        }

        if (!envelope_.isActive())
            clearCurrentNote();
    }

    void setCurrentPlaybackSampleRate(double sampleRate) override {
        SynthesiserVoice::setCurrentPlaybackSampleRate(sampleRate);
        if (std::isfinite(sampleRate) && sampleRate > 0.0)
            currentSampleRate_ = sampleRate;
        filterParametersInitialized_ = false;
        envelope_.setSampleRate(sampleRate);
        filterEnvelope_.setSampleRate(sampleRate);
        filter_.prepare(sampleRate);
        pas1::dsp::WavetableOscillator::initialize();
    }

private:
    pas1::dsp::EnvelopeParameters readFilterEnvelopeParameters() const {
        return {
            engineParameters_.filterAttack->load(),
            engineParameters_.filterDecay->load(),
            engineParameters_.filterSustain->load(),
            engineParameters_.filterRelease->load()
        };
    }

    float nextNoise() noexcept {
        noiseState_ ^= noiseState_ << 13;
        noiseState_ ^= noiseState_ >> 17;
        noiseState_ ^= noiseState_ << 5;
        return static_cast<float>(noiseState_ >> 8) / 8388607.5f - 1.0f;
    }

    void updateEnvelopeParameters() {
        envelope_.setParameters({
            attack_->load(),
            decay_->load(),
            sustain_->load(),
            release_->load()
        });
    }

    std::atomic<float>* waveform_;
    std::atomic<float>* attack_;
    std::atomic<float>* decay_;
    std::atomic<float>* sustain_;
    std::atomic<float>* release_;
    std::atomic<float>* polyphony_;
    std::atomic<float>* voiceMode_;
    std::atomic<float>* glideEnabled_;
    std::atomic<float>* glideTime_;
    EngineParameterPointers engineParameters_;
    pas1::dsp::ModulationAccumulator* modulationAccumulator_;
    int voiceIndex_;
    std::array<pas1::dsp::WavetableOscillator, 7> unisonOscillators_{};
    pas1::dsp::WavetableOscillator oscillator2_;
    pas1::dsp::Oscillator oscillator3_;
    pas1::dsp::Oscillator subOscillator_;
    pas1::dsp::MetallicTine musicBoxTine_;
    pas1::dsp::AncientInstrumentVoice ancientInstrument_;
    pas1::dsp::AmpEnvelope envelope_;
    pas1::dsp::AmpEnvelope filterEnvelope_;
    pas1::dsp::StateVariableFilter filter_;
    std::array<pas1::dsp::LowFrequencyOscillator, 4> lfos_{};
    pas1::dsp::Portamento portamento_;
    double currentSampleRate_ = 44100.0;
    float smoothedFilterLogCutoff_ = 10.0f;
    float smoothedFilterResonance_ = 0.0f;
    bool filterParametersInitialized_ = false;
    float velocity_ = 0.0f;
    float pitchBendRatio_ = 1.0f;
    float pitchBendNormalized_ = 0.0f;
    float modWheel_ = 0.0f;
    double vibratoPhase_ = 0.0;
    std::uint32_t noiseState_ = 0x6d2b79f5u;
    float randomPerNote_ = 0.0f;
    int midiNoteNumber_ = 60;
    unsigned long long noteAgeSamples_ = 0;
    bool monoStealPending_ = false;
    bool legatoForNextNote_ = false;
    bool modeResetInProgress_ = false;
};

class Pas1Synthesiser final : public juce::Synthesiser {
public:
    Pas1Synthesiser() {
        pitchWheelValues_.fill(8192);
    }

    void setVoiceModeParameter(std::atomic<float>* voiceMode) noexcept {
        voiceMode_ = voiceMode;
    }

    void updateVoiceModeFromParameter() {
        setVoiceMode(static_cast<int>(std::round(voiceMode_->load(std::memory_order_relaxed))));
    }

    void setVoiceMode(int mode) {
        mode = std::clamp(mode, 0, 2);
        if (mode == currentMode_)
            return;
        for (int index = 0; index < getNumVoices(); ++index) {
            if (auto* voice = dynamic_cast<Pas1Voice*>(getVoice(index))) {
                voice->setModeResetInProgress(true);
                stopVoice(voice, 0.0f, false);
                voice->setModeResetInProgress(false);
            }
        }
        heldNotes_.clear();
        sustainPedalsDown_.fill(false);
        sostenutoPedalsDown_.fill(false);
        currentMode_ = mode;
    }

    int renderBlock(juce::AudioBuffer<float>& output,
                    const juce::MidiBuffer& midi,
                    int startSample,
                    int numSamples) {
        // Keep event dispatch on the audio thread without JUCE Synthesiser's internal lock.
        const auto blockEnd = startSample + numSamples;
        auto cursor = startSample;
        auto maximumActiveVoiceCount = getActiveVoiceCount();
        auto event = midi.findNextSamplePosition(startSample);

        for (; event != midi.cend(); ++event) {
            const auto metadata = *event;
            const auto eventSample = metadata.samplePosition;
            if (eventSample > blockEnd)
                break;

            const auto renderEnd = std::clamp(eventSample, cursor, blockEnd);
            if (renderEnd > cursor) {
                maximumActiveVoiceCount =
                    std::max(maximumActiveVoiceCount, getActiveVoiceCount());
                renderVoices(output, cursor, renderEnd - cursor);
                maximumActiveVoiceCount =
                    std::max(maximumActiveVoiceCount, getActiveVoiceCount());
            }

            handleMidiEvent(metadata.getMessage());
            maximumActiveVoiceCount =
                std::max(maximumActiveVoiceCount, getActiveVoiceCount());
            cursor = renderEnd;
        }

        if (cursor < blockEnd) {
            maximumActiveVoiceCount =
                std::max(maximumActiveVoiceCount, getActiveVoiceCount());
            renderVoices(output, cursor, blockEnd - cursor);
            maximumActiveVoiceCount =
                std::max(maximumActiveVoiceCount, getActiveVoiceCount());
        }

        return maximumActiveVoiceCount;
    }

    int getActiveVoiceCount() const noexcept {
        auto activeVoices = 0;
        for (int index = 0; index < getNumVoices(); ++index) {
            const auto* voice = getVoice(index);
            if (voice != nullptr && voice->isVoiceActive())
                ++activeVoices;
        }
        return activeVoices;
    }

    void handleMidiEvent(const juce::MidiMessage& message) override {
        const auto channel = message.getChannel();
        if (message.isAllNotesOff() || message.isAllSoundOff()) {
            heldNotes_.clear();
            stopNotesOnChannel(channel, message.isAllSoundOff());
            return;
        }

        if (message.isNoteOn()) {
            if (currentMode_ == 0) {
                startNote(channel, message.getNoteNumber(), message.getFloatVelocity(), false);
            } else {
                const auto wasLegato = heldNotes_.noteOn({
                    channel, message.getNoteNumber(), message.getFloatVelocity()
                });
                auto* voice = getMonoVoice();
                const auto repeatsCurrentNote =
                    voice != nullptr && voice->getCurrentlyPlayingNote() == message.getNoteNumber()
                    && voice->isPlayingChannel(channel);
                startNote(channel, message.getNoteNumber(), message.getFloatVelocity(),
                          wasLegato && !repeatsCurrentNote);
            }
            return;
        }

        if (message.isNoteOff()) {
            if (currentMode_ == 0) {
                stopNote(channel, message.getNoteNumber(), message.getFloatVelocity());
                return;
            }

            const auto wasTracked = heldNotes_.noteOff(channel, message.getNoteNumber());
            auto* voice = getMonoVoice();
            const auto isCurrentVoiceNote =
                voice != nullptr && voice->getCurrentlyPlayingNote() == message.getNoteNumber()
                && voice->isPlayingChannel(channel);
            if (!isCurrentVoiceNote) {
                if (!wasTracked)
                    stopNote(channel, message.getNoteNumber(), message.getFloatVelocity());
                return;
            }
            if (const auto* fallback = heldNotes_.mostRecent()) {
                startNote(fallback->channel, fallback->note, fallback->velocity, true);
                return;
            }
            stopNote(channel, message.getNoteNumber(), message.getFloatVelocity());
            return;
        }

        if (message.isPitchWheel()) {
            if (channel >= 1 && channel <= 16) {
                pitchWheelValues_[static_cast<std::size_t>(channel - 1)] =
                    message.getPitchWheelValue();
                forEachVoiceOnChannel(channel, [&message](juce::SynthesiserVoice& voice) {
                    voice.pitchWheelMoved(message.getPitchWheelValue());
                });
            }
            return;
        }

        if (message.isController()) {
            const auto controller = message.getControllerNumber();
            const auto value = message.getControllerValue();
            if (controller == 64) {
                setSustainPedal(channel, message.isSustainPedalOn());
                return;
            }
            if (controller == 66) {
                setSostenutoPedal(channel, message.isSostenutoPedalOn());
                return;
            }
            forEachVoiceOnChannel(channel, [controller, value](juce::SynthesiserVoice& voice) {
                voice.controllerMoved(controller, value);
            });
            return;
        }

        if (message.isAftertouch()) {
            const auto note = message.getNoteNumber();
            forEachVoiceOnChannel(channel, [note, &message](juce::SynthesiserVoice& voice) {
                if (voice.getCurrentlyPlayingNote() == note)
                    voice.aftertouchChanged(message.getAfterTouchValue());
            });
            return;
        }

        if (message.isChannelPressure()) {
            forEachVoiceOnChannel(channel, [&message](juce::SynthesiserVoice& voice) {
                voice.channelPressureChanged(message.getChannelPressureValue());
            });
        }
    }

private:
    Pas1Voice* getMonoVoice() const noexcept {
        return getNumVoices() > 0 ? dynamic_cast<Pas1Voice*>(getVoice(0)) : nullptr;
    }

    template <typename Callback>
    void forEachVoiceOnChannel(int channel, Callback&& callback) {
        for (int index = 0; index < getNumVoices(); ++index) {
            if (auto* voice = getVoice(index);
                voice != nullptr && voice->isVoiceActive()
                && (channel <= 0 || voice->isPlayingChannel(channel)))
                callback(*voice);
        }
    }

    void startNote(int channel, int note, float velocity, bool isLegato) {
        if (channel < 1 || channel > 16 || getNumSounds() == 0)
            return;
        const auto sound = getSound(0);
        auto* const rawSound = sound.get();
        if (rawSound == nullptr || !rawSound->appliesToNote(note)
            || !rawSound->appliesToChannel(channel))
            return;

        auto* voice = findFreeVoice(rawSound, channel, note, true);
        if (auto* pasVoice = dynamic_cast<Pas1Voice*>(voice))
            pasVoice->setLegatoForNextNote(isLegato);
        if (voice == nullptr)
            return;

        startVoice(voice, rawSound, channel, note, velocity);
        voice->setSustainPedalDown(sustainPedalsDown_[static_cast<std::size_t>(channel)]);
        voice->pitchWheelMoved(pitchWheelValues_[static_cast<std::size_t>(channel - 1)]);
    }

    void stopNote(int channel, int note, float velocity) {
        for (int index = 0; index < getNumVoices(); ++index) {
            auto* voice = getVoice(index);
            if (voice == nullptr || !voice->isVoiceActive()
                || voice->getCurrentlyPlayingNote() != note
                || (channel > 0 && !voice->isPlayingChannel(channel)))
                continue;

            voice->setKeyDown(false);
            if (voice->isSostenutoPedalDown()
                || (channel >= 1 && channel <= 16
                    && sustainPedalsDown_[static_cast<std::size_t>(channel)]))
                continue;
            stopVoice(voice, velocity, true);
        }
    }

    void stopNotesOnChannel(int channel, bool immediate) {
        forEachVoiceOnChannel(channel, [this, channel, immediate](juce::SynthesiserVoice& voice) {
            voice.setKeyDown(false);
            if (!immediate && voice.isSostenutoPedalDown())
                return;
            if (!immediate && channel >= 1 && channel <= 16
                && sustainPedalsDown_[static_cast<std::size_t>(channel)]) {
                voice.setSustainPedalDown(true);
                return;
            }
            stopVoice(&voice, 1.0f, !immediate);
        });
    }

    void setSustainPedal(int channel, bool isDown) {
        if (channel < 1 || channel > 16)
            return;
        sustainPedalsDown_[static_cast<std::size_t>(channel)] = isDown;
        forEachVoiceOnChannel(channel, [this, isDown](juce::SynthesiserVoice& voice) {
            voice.setSustainPedalDown(isDown);
            if (!isDown && voice.isPlayingButReleased())
                stopVoice(&voice, 0.0f, true);
        });
    }

    void setSostenutoPedal(int channel, bool isDown) {
        if (channel < 1 || channel > 16)
            return;
        const auto index = static_cast<std::size_t>(channel);
        const auto wasDown = sostenutoPedalsDown_[index];
        sostenutoPedalsDown_[index] = isDown;
        forEachVoiceOnChannel(channel, [this, isDown, wasDown](juce::SynthesiserVoice& voice) {
            if (isDown && !wasDown && voice.isKeyDown())
                voice.setSostenutoPedalDown(true);
            else if (!isDown) {
                voice.setSostenutoPedalDown(false);
                if (voice.isPlayingButReleased() && !voice.isSustainPedalDown())
                    stopVoice(&voice, 0.0f, true);
            }
        });
    }

    std::atomic<float>* voiceMode_ = nullptr;
    pas1::dsp::NotePriorityStack heldNotes_;
    std::array<bool, 17> sustainPedalsDown_{};
    std::array<bool, 17> sostenutoPedalsDown_{};
    std::array<int, 16> pitchWheelValues_{};
    int currentMode_ = -1;
};

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout Pas1AudioProcessor::createParameterLayout() {
    using Parameter = Utf8NamedParameter<juce::AudioParameterFloat>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"waveform", 1},
        "Форма волны",
        pas1::ui::utf8StringArray({"Синус", "Треугольник", "Пила", "Прямоугольник",
                                   "Импульс", "Обратная пила", "Орган", "Полый",
                                   "Формант", "Колокол", "Металл", "Мягкий клиппинг"}),
        2));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc1level", 1}, "Уровень осц. 1",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.8f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterInt>>(
        juce::ParameterID{"osc1octave", 1}, "Октава осц. 1", -3, 3, 0));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc1fine", 1}, "Точная настройка осц. 1",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 0.1f}, 0.0f,
        pas1::ui::utf8String(" центов")));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc1phase", 1}, "Фаза осц. 1",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"wavetablescan", 1}, "Позиция волновой таблицы",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterInt>>(
        juce::ParameterID{"unisonvoices", 1}, "Голоса унисона", 1, 7, 1));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"unisondetune", 1}, "Расстройка унисона",
        juce::NormalisableRange<float>{0.0f, 50.0f, 0.1f}, 12.0f,
        pas1::ui::utf8String(" центов")));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"osc2wave", 1}, "Форма волны осц. 2",
        pas1::ui::utf8StringArray({"Синус", "Треугольник", "Пила", "Прямоугольник",
                                   "Импульс", "Обратная пила", "Орган", "Полый",
                                   "Формант", "Колокол", "Металл", "Мягкий клиппинг"}), 0));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc2level", 1}, "Уровень осц. 2",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.2f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc2detune", 1}, "Расстройка осц. 2",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 0.1f}, 0.0f,
        pas1::ui::utf8String(" центов")));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc2phase", 1}, "Смещение фазы осц. 2",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"osc2sync", 1}, "Жёсткая синхронизация осц. 2", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"fmamount", 1}, "Глубина ЧМ",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"ringamount", 1}, "Глубина кольцевой модуляции",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"osc3wave", 1}, "Форма волны осц. 3",
        pas1::ui::utf8StringArray({"Синус", "Треугольник", "Пила", "Прямоугольник",
                                   "Импульс", "Обратная пила", "Орган", "Полый",
                                   "Формант", "Колокол", "Металл", "Мягкий клиппинг"}), 1));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"osc3level", 1}, "Уровень осц. 3",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"pulsewidth", 1}, "Ширина импульса",
        juce::NormalisableRange<float>{0.05f, 0.95f, 0.001f}, 0.5f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"sublevel", 1}, "Уровень саб-низа",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"noiselevel", 1}, "Уровень шума",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"musicboxmix", 1}, "Уровень металлических язычков",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"musicboxbrightness", 1}, "Яркость металлических язычков",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.72f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"musicboxdecay", 1}, "Затухание металлических язычков",
        juce::NormalisableRange<float>{0.08f, 8.0f, 0.001f, 0.4f}, 1.4f, " s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"musicboxresonance", 1}, "Резонанс металлических язычков",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.62f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"musicboxinharmonicity", 1}, "Негармоничность обертонов",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.72f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"musicboxstrike", 1}, "Характер удара",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.58f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"ancientinstrument", 1}, "Древний инструмент",
        pas1::ui::utf8StringArray({"Выкл.", "Аулос", "Лира", "Бронзовый гонг", "Найденный металл"}), 0));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"ancientmix", 1}, "Уровень древнего тембра",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.78f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"ancientbrightness", 1}, "Яркость древнего тембра",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.68f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"ancientdecay", 1}, "Затухание древнего тембра",
        juce::NormalisableRange<float>{0.08f, 20.0f, 0.001f, 0.4f}, 3.2f, " s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"ancientair", 1}, "Воздух / атака древнего тембра",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.42f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"ancientdetune", 1}, "Расстройка трубок аулоса",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.32f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"filtertype", 1}, "Тип фильтра",
        pas1::ui::utf8StringArray({"Низких частот", "Высоких частот", "Полосовой", "Режекторный"}), 0));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filtercutoff", 1}, "Частота среза фильтра",
        juce::NormalisableRange<float>{20.0f, 20000.0f, 0.1f, 0.25f}, 18000.0f, " Hz"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterresonance", 1}, "Резонанс фильтра",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.15f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterdrive", 1}, "Насыщение фильтра",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterenvamount", 1}, "Глубина огибающей фильтра",
        juce::NormalisableRange<float>{-1.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterkeytracking", 1}, "Отслеживание клавиатуры фильтром",
        juce::NormalisableRange<float>{-1.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filtervelocity", 1}, "Влияние скорости нажатия на фильтр",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filtermodwheel", 1}, "Влияние колеса модуляции на фильтр",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterrandom", 1}, "Случайная модуляция фильтра для каждой ноты",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterattack", 1}, "Атака фильтра",
        juce::NormalisableRange<float>{0.001f, 5.0f, 0.001f, 0.35f}, 0.01f, " s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterdecay", 1}, "Спад фильтра",
        juce::NormalisableRange<float>{0.001f, 5.0f, 0.001f, 0.35f}, 0.2f, " s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filtersustain", 1}, "Удержание фильтра",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.75f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"filterrelease", 1}, "Затухание фильтра",
        juce::NormalisableRange<float>{0.01f, 10.0f, 0.001f, 0.35f}, 0.5f, " s"));
    for (int index = 0; index < 4; ++index) {
        const auto prefix = "lfo" + std::to_string(index + 1);
        layout.add(std::make_unique<Parameter>(
            juce::ParameterID{prefix + "rate", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — скорость"),
            juce::NormalisableRange<float>{0.01f, 20.0f, 0.001f, 0.35f},
            index == 0 ? 5.0f : 1.0f, " Hz"));
        layout.add(std::make_unique<Parameter>(
            juce::ParameterID{prefix + "amount", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — глубина"),
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
            juce::ParameterID{prefix + "wave", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — форма волны"),
            pas1::ui::utf8StringArray({"Синус", "Треугольник", "Пила", "Прямоугольник", "Выборка и удержание", "Плавный случайный"}), 0));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
            juce::ParameterID{prefix + "target", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — цель"),
            pas1::ui::utf8StringArray({"Выкл.", "Высота тона", "Частота среза фильтра",
                                       "Уровень осц. 2", "Громкость"}), 0));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
            juce::ParameterID{prefix + "sync", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — синхронизация"), false));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
            juce::ParameterID{prefix + "division", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — деление"),
            juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16"}, 2));
        layout.add(std::make_unique<Parameter>(
            juce::ParameterID{prefix + "phase", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — фаза"),
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
        layout.add(std::make_unique<Parameter>(
            juce::ParameterID{prefix + "fade", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — нарастание"),
            juce::NormalisableRange<float>{0.0f, 10.0f, 0.001f, 0.35f}, 0.0f, " s"));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
            juce::ParameterID{prefix + "retrigger", 1}, pas1::ui::utf8String("НЧ-генератор ") + juce::String(index + 1) + pas1::ui::utf8String(" — перезапуск"),
            true));
    }
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"attack", 1}, "Атака",
        juce::NormalisableRange<float>{0.001f, 5.0f, 0.001f, 0.35f}, 0.01f, "s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"decay", 1}, "Спад",
        juce::NormalisableRange<float>{0.001f, 5.0f, 0.001f, 0.35f}, 0.2f, "s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"sustain", 1}, "Удержание",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.75f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"release", 1}, "Затухание",
        juce::NormalisableRange<float>{0.01f, 10.0f, 0.001f, 0.35f}, 0.5f, "s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"master", 1}, "Общая громкость",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.75f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"macrobody", 1}, "Макро — Тело",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 50.0f, "%"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"macrobrightness", 1}, "Макро — Яркость",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 50.0f, "%"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"macromovement", 1}, "Макро — Движение",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 50.0f, "%"));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterInt>>(
        juce::ParameterID{"tuningoctave", 1}, "Глобальный сдвиг октавы", -3, 3, 0));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterInt>>(
        juce::ParameterID{"transpose", 1}, "Транспонирование", -24, 24, 0));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"masterfine", 1}, "Точная настройка",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 0.1f}, 0.0f,
        pas1::ui::utf8String(" центов")));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"temperament", 1}, "Строй",
        pas1::ui::utf8StringArray(
            {"Равномерный (12-TET)", "Чистый", "Пифагорейский", "19-TET"}),
        0));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"tuningroot", 1}, "Тоника строя",
        pas1::ui::utf8StringArray(
            {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}),
        0));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterInt>>(
        juce::ParameterID{"polyphony", 1}, "Число нот", 1, kMaximumVoices, 16));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
        juce::ParameterID{"voicemode", 1}, "Режим голосов",
        pas1::ui::utf8StringArray({"Полифония", "Монофония", "Легато"}), 0));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"glideenabled", 1}, "Глиссандо включено", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"glidetime", 1}, "Время глиссандо",
        juce::NormalisableRange<float>{0.001f, 2.0f, 0.001f, 0.4f}, 0.1f, " s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"morphx", 1}, "Морфинг X",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"morphy", 1}, "Морфинг Y",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"stereowidth", 1}, "Ширина стерео",
        juce::NormalisableRange<float>{0.0f, 2.0f, 0.001f}, 1.0f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"distortionenabled", 1}, "Искажение включено", true));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"distortionmix", 1}, "Уровень искажения",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"distortiondrive", 1}, "Насыщение искажения",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.25f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"chorusenabled", 1}, "Хорус включён", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"chorusmix", 1}, "Уровень хоруса",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"chorusrate", 1}, "Скорость хоруса",
        juce::NormalisableRange<float>{0.01f, 8.0f, 0.001f, 0.35f}, 0.8f, " Hz"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"chorusdepth", 1}, "Глубина хоруса",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.5f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"phaserenabled", 1}, "Фазер включён", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"phasermix", 1}, "Уровень фазера",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"phaserrate", 1}, "Скорость фазера",
        juce::NormalisableRange<float>{0.01f, 8.0f, 0.001f, 0.35f}, 0.25f, " Hz"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"phaserdepth", 1}, "Глубина фазера",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.5f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"delayenabled", 1}, "Задержка включена", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"delaymix", 1}, "Уровень задержки",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"delaytime", 1}, "Время задержки",
        juce::NormalisableRange<float>{0.01f, 2.45f, 0.001f, 0.35f}, 0.35f, " s"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"delayfeedback", 1}, "Обратная связь задержки",
        juce::NormalisableRange<float>{0.0f, 0.92f, 0.001f}, 0.25f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"reverbenabled", 1}, "Реверберация включена", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"reverbmix", 1}, "Уровень реверберации",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"reverbdecay", 1}, "Затухание реверберации",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.5f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"reverbdamping", 1}, "Демпфирование реверберации",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 0.5f));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"eqenabled", 1}, "Эквалайзер включён", false));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"eqmix", 1}, "Уровень эквалайзера",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f}, 1.0f));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"eqlowgain", 1}, "Усиление низких частот",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f, " dB"));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"eqhighgain", 1}, "Усиление высоких частот",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f, " dB"));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
        juce::ParameterID{"limiterenabled", 1}, "Выходной лимитер включён", true));
    layout.add(std::make_unique<Parameter>(
        juce::ParameterID{"limiterthreshold", 1}, "Порог лимитера",
        juce::NormalisableRange<float>{0.5f, 0.999f, 0.001f}, 0.92f));

    juce::StringArray modulationSources;
    for (const auto* name : kModulationSourceNames)
        modulationSources.add(pas1::ui::utf8String(name));
    juce::StringArray modulationDestinations;
    for (const auto* name : kModulationDestinationNames)
        modulationDestinations.add(pas1::ui::utf8String(name));
    for (std::size_t index = 0; index < pas1::dsp::ModulationMatrix::routeCount; ++index) {
        const auto prefix = "modroute" + std::to_string(index + 1);
        const auto routeName = pas1::ui::utf8String("Маршрут модуляции ") + juce::String(static_cast<int>(index + 1));
    layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
            juce::ParameterID{prefix + "source", 1}, routeName + pas1::ui::utf8String(" — источник"),
            modulationSources, 0));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
            juce::ParameterID{prefix + "destination", 1}, routeName + pas1::ui::utf8String(" — цель"),
            modulationDestinations, 0));
        layout.add(std::make_unique<Parameter>(
            juce::ParameterID{prefix + "amount", 1}, routeName + pas1::ui::utf8String(" — глубина"),
            juce::NormalisableRange<float>{-1.0f, 1.0f, 0.001f}, 0.0f));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterChoice>>(
            juce::ParameterID{prefix + "polarity", 1}, routeName + pas1::ui::utf8String(" — полярность"),
            pas1::ui::utf8StringArray({"Биполярная", "Униполярная"}), 0));
        layout.add(std::make_unique<Utf8NamedParameter<juce::AudioParameterBool>>(
            juce::ParameterID{prefix + "enabled", 1}, routeName + pas1::ui::utf8String(" — включён"), false));
    }
    return layout;
}

Pas1AudioProcessor::Pas1AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PAS1", createParameterLayout()),
      presetManager(parameters) {
    keyboardState.addListener(this);
    synthesiser = std::make_unique<Pas1Synthesiser>();
    presetManager.setStateLoadedCallback([this] { readMorphState(parameters.state); });
    pas1::dsp::WavetableOscillator::initialize();
    EngineParameterPointers engineParameters;
    engineParameters.oscillator1Level = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::oscillator1Level)];
    engineParameters.oscillator1Octave = parameters.getRawParameterValue("osc1octave");
    engineParameters.oscillator1Fine = parameters.getRawParameterValue("osc1fine");
    engineParameters.oscillator1Phase = parameters.getRawParameterValue("osc1phase");
    engineParameters.wavetableScan = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::wavetableScan)];
    engineParameters.unisonVoices = parameters.getRawParameterValue("unisonvoices");
    engineParameters.unisonDetune = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::unisonDetune)];
    engineParameters.oscillator2Waveform = parameters.getRawParameterValue("osc2wave");
    engineParameters.oscillator2Level = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::oscillator2Level)];
    engineParameters.oscillator2Detune = parameters.getRawParameterValue("osc2detune");
    engineParameters.oscillator2Phase = parameters.getRawParameterValue("osc2phase");
    engineParameters.oscillator2HardSync = parameters.getRawParameterValue("osc2sync");
    engineParameters.oscillator2FmAmount = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::fmAmount)];
    engineParameters.oscillator2RingAmount = parameters.getRawParameterValue("ringamount");
    engineParameters.oscillator3Waveform = parameters.getRawParameterValue("osc3wave");
    engineParameters.oscillator3Level = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::oscillator3Level)];
    engineParameters.pulseWidth = parameters.getRawParameterValue("pulsewidth");
    engineParameters.subLevel = parameters.getRawParameterValue("sublevel");
    engineParameters.noiseLevel = parameters.getRawParameterValue("noiselevel");
    engineParameters.musicBoxMix = parameters.getRawParameterValue("musicboxmix");
    engineParameters.musicBoxBrightness = parameters.getRawParameterValue("musicboxbrightness");
    engineParameters.musicBoxDecay = parameters.getRawParameterValue("musicboxdecay");
    engineParameters.musicBoxResonance = parameters.getRawParameterValue("musicboxresonance");
    engineParameters.musicBoxInharmonicity =
        parameters.getRawParameterValue("musicboxinharmonicity");
    engineParameters.musicBoxStrike = parameters.getRawParameterValue("musicboxstrike");
    engineParameters.ancientInstrument = parameters.getRawParameterValue("ancientinstrument");
    engineParameters.ancientMix = parameters.getRawParameterValue("ancientmix");
    engineParameters.ancientBrightness = parameters.getRawParameterValue("ancientbrightness");
    engineParameters.ancientDecay = parameters.getRawParameterValue("ancientdecay");
    engineParameters.ancientAir = parameters.getRawParameterValue("ancientair");
    engineParameters.ancientPipeDetune = parameters.getRawParameterValue("ancientdetune");
    engineParameters.macroBody = parameters.getRawParameterValue("macrobody");
    engineParameters.macroBrightness = parameters.getRawParameterValue("macrobrightness");
    engineParameters.macroMovement = parameters.getRawParameterValue("macromovement");
    engineParameters.tuningOctave = parameters.getRawParameterValue("tuningoctave");
    engineParameters.transpose = parameters.getRawParameterValue("transpose");
    engineParameters.masterFineTune = parameters.getRawParameterValue("masterfine");
    engineParameters.temperament = parameters.getRawParameterValue("temperament");
    engineParameters.tuningRoot = parameters.getRawParameterValue("tuningroot");
    engineParameters.filterType = parameters.getRawParameterValue("filtertype");
    engineParameters.filterCutoff = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::filterCutoff)];
    engineParameters.filterResonance = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::filterResonance)];
    engineParameters.filterDrive = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::filterDrive)];
    engineParameters.filterEnvelopeAmount = parameters.getRawParameterValue("filterenvamount");
    engineParameters.filterKeyTracking = parameters.getRawParameterValue("filterkeytracking");
    engineParameters.filterVelocityAmount = parameters.getRawParameterValue("filtervelocity");
    engineParameters.filterModWheelAmount = parameters.getRawParameterValue("filtermodwheel");
    engineParameters.filterRandomAmount = parameters.getRawParameterValue("filterrandom");
    engineParameters.filterAttack = parameters.getRawParameterValue("filterattack");
    engineParameters.filterDecay = parameters.getRawParameterValue("filterdecay");
    engineParameters.filterSustain = parameters.getRawParameterValue("filtersustain");
    engineParameters.filterRelease = parameters.getRawParameterValue("filterrelease");
    engineParameters.hostTempoBpm = &hostTempoBpm;
    for (int index = 0; index < 4; ++index) {
        const auto prefix = "lfo" + std::to_string(index + 1);
        auto& lfo = engineParameters.lfos[static_cast<std::size_t>(index)];
        lfo.rate = parameters.getRawParameterValue(prefix + "rate");
        lfo.amount = parameters.getRawParameterValue(prefix + "amount");
        lfo.waveform = parameters.getRawParameterValue(prefix + "wave");
        lfo.destination = parameters.getRawParameterValue(prefix + "target");
        lfo.sync = parameters.getRawParameterValue(prefix + "sync");
        lfo.division = parameters.getRawParameterValue(prefix + "division");
        lfo.phase = parameters.getRawParameterValue(prefix + "phase");
        lfo.fadeIn = parameters.getRawParameterValue(prefix + "fade");
        lfo.retrigger = parameters.getRawParameterValue(prefix + "retrigger");
    }
    engineParameters.lfos[0].amount =
        &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::lfoDepth)];
    for (std::size_t index = 0; index < engineParameters.modulationRoutes.size(); ++index) {
        const auto prefix = "modroute" + std::to_string(index + 1);
        auto& route = engineParameters.modulationRoutes[index];
        route.source = parameters.getRawParameterValue(prefix + "source");
        route.destination = parameters.getRawParameterValue(prefix + "destination");
        route.amount = parameters.getRawParameterValue(prefix + "amount");
        route.polarity = parameters.getRawParameterValue(prefix + "polarity");
        route.enabled = parameters.getRawParameterValue(prefix + "enabled");
    }

    auto* waveform = parameters.getRawParameterValue("waveform");
    auto* attack = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::attack)];
    auto* decay = parameters.getRawParameterValue("decay");
    auto* sustain = parameters.getRawParameterValue("sustain");
    auto* release = &morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::release)];
    auto* polyphony = parameters.getRawParameterValue("polyphony");
    auto* voiceMode = parameters.getRawParameterValue("voicemode");
    auto* glideEnabled = parameters.getRawParameterValue("glideenabled");
    auto* glideTime = parameters.getRawParameterValue("glidetime");
    static_cast<Pas1Synthesiser*>(synthesiser.get())->setVoiceModeParameter(voiceMode);

    for (int voice = 0; voice < kMaximumVoices; ++voice)
        synthesiser->addVoice(new Pas1Voice(waveform, attack, decay, sustain, release, polyphony,
                                            voiceMode, glideEnabled, glideTime, engineParameters,
                                            &modulationAccumulator, voice));
    synthesiser->addSound(new Pas1Sound());
    masterGain.reset(44100.0, 0.05);
}

Pas1AudioProcessor::~Pas1AudioProcessor() {
    keyboardState.removeListener(this);
}

void Pas1AudioProcessor::handleNoteOn(juce::MidiKeyboardState*,
                                      int channel,
                                      int note,
                                      float velocity) {
    // MidiKeyboardState invokes listeners synchronously; keep this callback non-blocking.
    enqueueKeyboardEvent({channel, note, velocity, true});
}

void Pas1AudioProcessor::handleNoteOff(juce::MidiKeyboardState*,
                                       int channel,
                                       int note,
                                       float velocity) {
    enqueueKeyboardEvent({channel, note, velocity, false});
}

void Pas1AudioProcessor::enqueueKeyboardEvent(KeyboardEvent event) noexcept {
    const auto writeIndex = keyboardEventWriteIndex.load(std::memory_order_relaxed);
    const auto readIndex = keyboardEventReadIndex.load(std::memory_order_acquire);
    if (writeIndex - readIndex >= keyboardEventCapacity) {
        keyboardEventOverflow.store(true, std::memory_order_release);
        return;
    }

    keyboardEvents[writeIndex % keyboardEventCapacity] = event;
    keyboardEventWriteIndex.store(writeIndex + 1, std::memory_order_release);
}

bool Pas1AudioProcessor::dequeueKeyboardEvent(KeyboardEvent& event) noexcept {
    const auto readIndex = keyboardEventReadIndex.load(std::memory_order_relaxed);
    const auto writeIndex = keyboardEventWriteIndex.load(std::memory_order_acquire);
    if (readIndex == writeIndex)
        return false;

    event = keyboardEvents[readIndex % keyboardEventCapacity];
    keyboardEventReadIndex.store(readIndex + 1, std::memory_order_release);
    return true;
}

void Pas1AudioProcessor::prepareToPlay(double sampleRate, int) {
    const auto validSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0
                                     ? sampleRate
                                     : 44100.0;
    synthesiser->setCurrentPlaybackSampleRate(validSampleRate);
    effectsChain.prepare(validSampleRate);
    morphSampleRate = validSampleRate;
    visualSampleRate.store(static_cast<float>(validSampleRate), std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
    midiActivity.store(0.0f, std::memory_order_relaxed);
    masterGain.reset(validSampleRate, 0.05);
    masterGain.setCurrentAndTargetValue(parameters.getRawParameterValue("master")->load());
}

void Pas1AudioProcessor::releaseResources() {}

bool Pas1AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto output = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
           && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void Pas1AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    if (auto* playHead = getPlayHead()) {
        if (const auto position = playHead->getPosition()) {
            if (const auto bpm = position->getBpm()) {
                if (std::isfinite(*bpm) && *bpm > 0.0)
                    hostTempoBpm.store(static_cast<float>(*bpm), std::memory_order_relaxed);
            }
        }
    }
    const auto samples = buffer.getNumSamples();
    const auto midiLevel = midi.getNumEvents() > 0
                               ? 1.0f
                               : midiActivity.load(std::memory_order_relaxed)
                                     * static_cast<float>(std::exp(
                                           -static_cast<double>(samples)
                                           / (std::max(1.0, morphSampleRate) * 0.18)));
    midiActivity.store(midiLevel, std::memory_order_relaxed);
    buffer.clear();
    const auto getValue = [this](const char* id) {
        return parameters.getRawParameterValue(id)->load(std::memory_order_relaxed);
    };
    auto* pas1Synthesiser = static_cast<Pas1Synthesiser*>(synthesiser.get());
    pas1Synthesiser->updateVoiceModeFromParameter();
    // Do not call MidiKeyboardState::processNextMidiBuffer here: JUCE takes a lock
    // and may grow its MidiBuffer. UI keyboard notes use the fixed SPSC queue instead.
    if (keyboardEventOverflow.exchange(false, std::memory_order_acq_rel)) {
        for (int channel = 1; channel <= 16; ++channel)
            pas1Synthesiser->handleMidiEvent(juce::MidiMessage::allSoundOff(channel));
        keyboardEventReadIndex.store(
            keyboardEventWriteIndex.load(std::memory_order_acquire),
            std::memory_order_release);
    }
    KeyboardEvent keyboardEvent;
    while (dequeueKeyboardEvent(keyboardEvent)) {
        const auto message = keyboardEvent.isNoteOn
                                 ? juce::MidiMessage::noteOn(keyboardEvent.channel,
                                                             keyboardEvent.note,
                                                             keyboardEvent.velocity)
                                 : juce::MidiMessage::noteOff(keyboardEvent.channel,
                                                              keyboardEvent.note,
                                                              keyboardEvent.velocity);
        pas1Synthesiser->handleMidiEvent(message);
    }
    modulationAccumulator.reset();
    updateMorphValues(samples);
    const auto activeVoiceCount =
        std::max(1, pas1Synthesiser->renderBlock(buffer, midi, 0, samples));
    // Only reserve headroom for notes that actually overlap this block. The polyphony
    // setting is a capacity limit and must not attenuate a lone note by default.
    const auto polyphonyHeadroom = 1.0f / std::sqrt(static_cast<float>(activeVoiceCount));
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < samples; ++sample)
            channelData[sample] *= polyphonyHeadroom;
    }
    const auto blockModulation = modulationAccumulator.average();
    modulationMorphX =
        blockModulation[static_cast<std::size_t>(pas1::dsp::ModulationDestination::morphX)] * 0.5f;
    modulationMorphY =
        blockModulation[static_cast<std::size_t>(pas1::dsp::ModulationDestination::morphY)] * 0.5f;

    const auto isEnabled = [&getValue](const char* id) { return getValue(id) >= 0.5f; };
    pas1::dsp::EffectsParameters effectParameters;
    effectParameters.distortionEnabled = isEnabled("distortionenabled");
    effectParameters.distortionMix = std::clamp(
        morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::distortionMix)]
                .load(std::memory_order_relaxed)
            + blockModulation[static_cast<std::size_t>(
                pas1::dsp::ModulationDestination::distortionMix)] * 0.5f,
        0.0f, 1.0f);
    effectParameters.distortionDrive = getValue("distortiondrive");
    effectParameters.chorusEnabled = isEnabled("chorusenabled");
    effectParameters.stereoWidth =
        morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::stereoWidth)].load(std::memory_order_relaxed);
    effectParameters.chorusMix = getValue("chorusmix");
    effectParameters.chorusRateHz = getValue("chorusrate");
    effectParameters.chorusDepth = getValue("chorusdepth");
    effectParameters.phaserEnabled = isEnabled("phaserenabled");
    effectParameters.phaserMix = getValue("phasermix");
    effectParameters.phaserRateHz = getValue("phaserrate");
    effectParameters.phaserDepth = getValue("phaserdepth");
    effectParameters.delayEnabled = isEnabled("delayenabled");
    effectParameters.delayMix = getValue("delaymix");
    effectParameters.delayTimeSeconds = getValue("delaytime");
    effectParameters.delayFeedback = getValue("delayfeedback");
    effectParameters.reverbEnabled = isEnabled("reverbenabled");
    effectParameters.reverbMix = std::clamp(
        morphValues[static_cast<std::size_t>(pas1::dsp::MorphParameter::reverbMix)]
                .load(std::memory_order_relaxed)
            + blockModulation[static_cast<std::size_t>(
                pas1::dsp::ModulationDestination::reverbMix)] * 0.5f,
        0.0f, 1.0f);
    effectParameters.reverbDecay = getValue("reverbdecay");
    effectParameters.reverbDamping = getValue("reverbdamping");
    effectParameters.eqEnabled = isEnabled("eqenabled");
    effectParameters.eqMix = getValue("eqmix");
    effectParameters.eqLowGainDb = getValue("eqlowgain");
    effectParameters.eqHighGainDb = getValue("eqhighgain");
    effectParameters.limiterEnabled = isEnabled("limiterenabled");
    effectParameters.limiterThreshold = getValue("limiterthreshold");
    effectsChain.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), samples, effectParameters);

    masterGain.setTargetValue(parameters.getRawParameterValue("master")->load());
    auto blockPeak = 0.0f;
    for (int sample = 0; sample < samples; ++sample) {
        const auto gain = masterGain.getNextValue();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto value = buffer.getSample(channel, sample) * gain;
            value = pas1::dsp::EffectsChain::limitSample(
                value, effectParameters.limiterEnabled, effectParameters.limiterThreshold);
            buffer.setSample(channel, sample, value);
            blockPeak = std::max(blockPeak, std::abs(value));
        }
    }
    auto visualWriteIndex = spectrumWriteIndex.load(std::memory_order_relaxed);
    const auto channelCount = buffer.getNumChannels();
    for (int sample = 0; sample < samples; ++sample) {
        float monoSample = 0.0f;
        for (int channel = 0; channel < channelCount; ++channel)
            monoSample += buffer.getSample(channel, sample);
        if (channelCount > 0)
            monoSample /= static_cast<float>(channelCount);
        if (!std::isfinite(monoSample))
            monoSample = 0.0f;
        spectrumCapture[static_cast<std::size_t>(visualWriteIndex % spectrumCaptureSize)]
            .store(monoSample, std::memory_order_relaxed);
        ++visualWriteIndex;
    }
    spectrumWriteIndex.store(visualWriteIndex, std::memory_order_release);
    outputPeak.store(blockPeak, std::memory_order_relaxed);
}

void Pas1AudioProcessor::copyRecentAudioSamples(float* destination, int numSamples) const noexcept {
    if (destination == nullptr || numSamples <= 0)
        return;

    std::fill_n(destination, numSamples, 0.0f);
    const auto latest = spectrumWriteIndex.load(std::memory_order_acquire);
    const auto available = std::min<std::uint64_t>(
        latest, static_cast<std::uint64_t>(numSamples));
    const auto firstIndex = latest - available;
    const auto destinationOffset = static_cast<std::size_t>(
        static_cast<std::uint64_t>(numSamples) - available);
    for (std::uint64_t index = 0; index < available; ++index) {
        const auto sourceIndex = (firstIndex + index) % spectrumCaptureSize;
        destination[destinationOffset + static_cast<std::size_t>(index)] =
            spectrumCapture[static_cast<std::size_t>(sourceIndex)].load(std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* Pas1AudioProcessor::createEditor() {
    return new Pas1AudioProcessorEditor(*this);
}

void Pas1AudioProcessor::getStateInformation(juce::MemoryBlock& destinationData) {
    auto state = parameters.copyState();
    writeMorphState(state);
    state.setProperty("stateVersion", 10, nullptr);
    if (const auto xml = state.createXml())
        copyXmlToBinary(*xml, destinationData);
}

void Pas1AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (const auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(parameters.state.getType())) {
            auto restoredState = juce::ValueTree::fromXml(*xml);
            constexpr std::array<const char*, 20> newParameterIds{
                "musicboxmix", "musicboxbrightness", "musicboxdecay",
                "musicboxresonance", "musicboxinharmonicity", "musicboxstrike",
                "ancientinstrument", "ancientmix", "ancientbrightness",
                "ancientdecay", "ancientair", "ancientdetune",
                "macrobody", "macrobrightness", "macromovement",
                "tuningoctave", "transpose", "masterfine", "temperament", "tuningroot"};
            for (const auto* parameterId : newParameterIds) {
                const juce::String id{parameterId};
                if (restoredState.getChildWithProperty("id", id).isValid())
                    continue;
                auto* parameter = parameters.getParameter(id);
                const auto currentParameterState =
                    parameters.state.getChildWithProperty("id", id);
                if (parameter == nullptr || !currentParameterState.isValid())
                    continue;
                auto migratedParameterState = currentParameterState.createCopy();
                migratedParameterState.setProperty(
                    "value", parameter->convertFrom0to1(parameter->getDefaultValue()), nullptr);
                restoredState.addChild(migratedParameterState, -1, nullptr);
            }
            const auto previousStateVersion =
                static_cast<int>(restoredState.getProperty("stateVersion", 0));
            if (previousStateVersion < 9) {
                auto resonanceState =
                    restoredState.getChildWithProperty("id", "filterresonance");
                if (resonanceState.isValid() && resonanceState.hasProperty("value")) {
                    const auto legacyResonance =
                        static_cast<float>(resonanceState.getProperty("value"));
                    resonanceState.setProperty(
                        "value", pas1::dsp::StateVariableFilter::migrateLegacyResonance(
                                     legacyResonance), nullptr);
                }
            }
            restoredState.setProperty("stateVersion", 10, nullptr);
            parameters.replaceState(restoredState);
            readMorphState(parameters.state);
        }
    }
}

void Pas1AudioProcessor::updateMorphValues(int blockSize) noexcept {
    pas1::dsp::MorphMatrix::Values current{};
    for (std::size_t index = 0; index < current.size(); ++index)
        current[index] = parameters.getRawParameterValue(kMorphParameterIds[index])
                             ->load(std::memory_order_relaxed);
    const auto x = std::clamp(
        parameters.getRawParameterValue("morphx")->load(std::memory_order_relaxed)
            + modulationMorphX,
        0.0f, 1.0f);
    const auto y = std::clamp(
        parameters.getRawParameterValue("morphy")->load(std::memory_order_relaxed)
            + modulationMorphY,
        0.0f, 1.0f);
    const auto values = morphMatrix.process(x, y, current, morphSampleRate, blockSize);
    for (std::size_t index = 0; index < values.size(); ++index)
        morphValues[index].store(values[index], std::memory_order_relaxed);
}

void Pas1AudioProcessor::captureMorphCorner(int corner) {
    if (corner < 0 || corner > 3)
        return;
    pas1::dsp::MorphMatrix::Values values{};
    for (std::size_t index = 0; index < values.size(); ++index)
        values[index] = parameters.getRawParameterValue(kMorphParameterIds[index])
                            ->load(std::memory_order_relaxed);
    morphMatrix.captureCorner(corner, values);
    writeMorphState(parameters.state);
}

void Pas1AudioProcessor::captureFactoryMorphCorners() {
    const auto originalState = parameters.copyState();
    constexpr std::array<int, 4> factoryPresets{0, 1, 4, 9};
    std::array<pas1::dsp::MorphMatrix::Values, 4> corners{};
    for (std::size_t corner = 0; corner < factoryPresets.size(); ++corner) {
        if (!presetManager.loadBuiltInPreset(factoryPresets[corner])) {
            parameters.replaceState(originalState);
            return;
        }
        for (std::size_t index = 0; index < corners[corner].size(); ++index)
            corners[corner][index] = parameters.getRawParameterValue(kMorphParameterIds[index])
                                         ->load(std::memory_order_relaxed);
    }
    parameters.replaceState(originalState);
    for (int corner = 0; corner < static_cast<int>(corners.size()); ++corner)
        morphMatrix.captureCorner(corner, corners[static_cast<std::size_t>(corner)]);
    writeMorphState(parameters.state);
}

void Pas1AudioProcessor::randomizeMorphCorners(std::uint32_t seed) {
    if (!morphMatrix.isEnabled())
        captureFactoryMorphCorners();
    morphMatrix.randomizeCorners(seed);
    writeMorphState(parameters.state);
}

void Pas1AudioProcessor::resetMorphConfiguration() {
    for (int index = parameters.state.getNumProperties() - 1; index >= 0; --index) {
        const auto property = parameters.state.getPropertyName(index);
        if (property.toString().startsWith("morph"))
            parameters.state.removeProperty(property, nullptr);
    }
    readMorphState(parameters.state);
}

void Pas1AudioProcessor::setMorphParameterLocked(int parameter, bool locked) {
    if (parameter < 0 || parameter >= static_cast<int>(pas1::dsp::MorphMatrix::parameterCount))
        return;
    morphMatrix.setLocked(static_cast<std::size_t>(parameter), locked);
    writeMorphState(parameters.state);
}

bool Pas1AudioProcessor::isMorphParameterLocked(int parameter) const {
    return parameter >= 0
           && parameter < static_cast<int>(pas1::dsp::MorphMatrix::parameterCount)
           && morphMatrix.isLocked(static_cast<std::size_t>(parameter));
}

bool Pas1AudioProcessor::isMorphConfigured() const noexcept {
    return morphMatrix.isEnabled();
}

juce::String Pas1AudioProcessor::getMorphParameterName(int parameter) {
    static const std::array<const char*, pas1::dsp::MorphMatrix::parameterCount> names{
        "Ур. осц. 1", "Ур. осц. 2", "Ур. осц. 3", "Позиция волны", "Расстройка",
        "Срез фильтра", "Резонанс", "Атака", "Затухание", "Глубина НЧГ",
        "Глубина ЧМ", "Насыщ. фильтра", "Искажение", "Ширина стерео", "Реверберация"};
    if (parameter < 0 || parameter >= static_cast<int>(names.size()))
        return {};
    return pas1::ui::utf8String(names[static_cast<std::size_t>(parameter)]);
}

void Pas1AudioProcessor::writeMorphState(juce::ValueTree& state) const {
    state.setProperty("morphEnabled", morphMatrix.isEnabled(), nullptr);
    for (int corner = 0; corner < 4; ++corner) {
        const auto values = morphMatrix.getCorner(corner);
        for (std::size_t index = 0; index < values.size(); ++index) {
            const auto property = "morphCorner" + juce::String(corner) + "_"
                                  + juce::String(static_cast<int>(index));
            state.setProperty(property, values[index], nullptr);
        }
    }
    for (std::size_t index = 0; index < pas1::dsp::MorphMatrix::parameterCount; ++index) {
        const auto property = "morphLocked_" + juce::String(static_cast<int>(index));
        state.setProperty(property, morphMatrix.isLocked(index), nullptr);
    }
}

void Pas1AudioProcessor::readMorphState(const juce::ValueTree& state) {
    bool hasSavedCorners = false;
    std::array<pas1::dsp::MorphMatrix::Values, 4> corners{};
    for (int corner = 0; corner < 4; ++corner) {
        for (std::size_t index = 0; index < corners[static_cast<std::size_t>(corner)].size(); ++index) {
            const auto property = "morphCorner" + juce::String(corner) + "_"
                                  + juce::String(static_cast<int>(index));
            if (state.hasProperty(property)) {
                corners[static_cast<std::size_t>(corner)][index] =
                    static_cast<float>(state.getProperty(property));
                hasSavedCorners = true;
            }
        }
    }
    if (hasSavedCorners) {
        for (int corner = 0; corner < 4; ++corner)
            morphMatrix.captureCorner(corner, corners[static_cast<std::size_t>(corner)]);
    }
    for (std::size_t index = 0; index < pas1::dsp::MorphMatrix::parameterCount; ++index) {
        const auto property = "morphLocked_" + juce::String(static_cast<int>(index));
        morphMatrix.setLocked(index, static_cast<bool>(state.getProperty(property, false)));
    }
    morphMatrix.setEnabled(
        hasSavedCorners && static_cast<bool>(state.getProperty("morphEnabled", false)));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new Pas1AudioProcessor();
}
