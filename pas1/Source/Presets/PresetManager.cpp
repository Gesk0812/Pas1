#include "PresetManager.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace {
struct ParameterSetting {
    const char* id;
    float value;
};

struct BuiltInPreset {
    const char* name;
    const char* category;
    const ParameterSetting* settings;
    std::size_t settingCount;
};

constexpr ParameterSetting subBassSettings[]{
    {"waveform", 3}, {"osc1level", 0.95f}, {"osc1octave", -1}, {"osc2wave", 0},
    {"osc2level", 0.18f}, {"osc3wave", 3}, {"osc3level", 0.0f}, {"sublevel", 0.65f},
    {"filtertype", 0}, {"filtercutoff", 190.0f}, {"filterresonance", 0.28f},
    {"filterenvamount", 0.42f}, {"attack", 0.005f}, {"decay", 0.26f},
    {"sustain", 0.72f}, {"release", 0.16f}, {"master", 0.7f}};
constexpr ParameterSetting reeseBassSettings[]{
    {"waveform", 2}, {"osc1level", 0.8f}, {"unisonvoices", 5}, {"unisondetune", 22.0f},
    {"osc2wave", 2}, {"osc2level", 0.45f}, {"osc2detune", -8.0f}, {"filtertype", 0},
    {"filtercutoff", 680.0f}, {"filterresonance", 0.52f}, {"chorusenabled", 1},
    {"chorusmix", 0.22f}, {"attack", 0.01f}, {"decay", 0.38f}, {"sustain", 0.76f},
    {"release", 0.24f}, {"master", 0.65f}};
constexpr ParameterSetting pluckBassSettings[]{
    {"waveform", 2}, {"osc1level", 0.85f}, {"osc1octave", -1}, {"osc2wave", 1},
    {"osc2level", 0.25f}, {"filtercutoff", 1200.0f}, {"filterenvamount", 0.75f},
    {"attack", 0.002f}, {"decay", 0.28f}, {"sustain", 0.18f}, {"release", 0.16f},
    {"delayenabled", 1}, {"delaymix", 0.12f}, {"master", 0.72f}};
constexpr ParameterSetting warmKeysSettings[]{
    {"waveform", 1}, {"osc1level", 0.72f}, {"osc2wave", 0}, {"osc2level", 0.28f},
    {"osc2detune", 7.0f}, {"osc3wave", 1}, {"osc3level", 0.18f}, {"filtercutoff", 5200.0f},
    {"attack", 0.012f}, {"decay", 0.45f}, {"sustain", 0.62f}, {"release", 0.55f},
    {"reverbenabled", 1}, {"reverbmix", 0.16f}, {"master", 0.68f}};
constexpr ParameterSetting glassKeysSettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 1}, {"osc2level", 0.25f},
    {"osc3wave", 0}, {"osc3level", 0.28f}, {"filtertype", 2}, {"filtercutoff", 4800.0f},
    {"attack", 0.004f}, {"decay", 0.7f}, {"sustain", 0.38f}, {"release", 0.7f},
    {"chorusenabled", 1}, {"chorusmix", 0.16f}, {"reverbenabled", 1},
    {"reverbmix", 0.2f}, {"master", 0.65f}};
constexpr ParameterSetting softPianoSettings[]{
    {"waveform", 1}, {"osc1level", 0.82f}, {"osc2wave", 2}, {"osc2level", 0.13f},
    {"filtercutoff", 3600.0f}, {"filterenvamount", 0.22f}, {"attack", 0.008f},
    {"decay", 0.62f}, {"sustain", 0.48f}, {"release", 0.65f}, {"master", 0.7f}};
constexpr ParameterSetting monoLeadSettings[]{
    {"waveform", 2}, {"osc1level", 0.75f}, {"osc2wave", 0}, {"osc2level", 0.32f},
    {"osc2detune", 5.0f}, {"filtercutoff", 4300.0f}, {"filterresonance", 0.24f},
    {"lfo1rate", 5.4f}, {"lfo1amount", 0.16f}, {"lfo1target", 1}, {"attack", 0.008f},
    {"decay", 0.24f}, {"sustain", 0.74f}, {"release", 0.21f}, {"master", 0.66f}};
constexpr ParameterSetting syncLeadSettings[]{
    {"waveform", 3}, {"osc1level", 0.76f}, {"osc2wave", 2}, {"osc2level", 0.38f},
    {"osc2sync", 1}, {"osc2detune", 18.0f}, {"filtercutoff", 5100.0f},
    {"filterresonance", 0.32f}, {"attack", 0.006f}, {"decay", 0.3f},
    {"sustain", 0.67f}, {"release", 0.25f}, {"master", 0.64f}};
constexpr ParameterSetting fmLeadSettings[]{
    {"waveform", 0}, {"osc1level", 0.8f}, {"osc2wave", 2}, {"osc2level", 0.38f},
    {"fmamount", 0.24f}, {"filtertype", 2}, {"filtercutoff", 4600.0f},
    {"attack", 0.003f}, {"decay", 0.38f}, {"sustain", 0.63f}, {"release", 0.22f},
    {"master", 0.67f}};
constexpr ParameterSetting slowPadSettings[]{
    {"waveform", 1}, {"osc1level", 0.75f}, {"unisonvoices", 7}, {"unisondetune", 34.0f},
    {"osc2wave", 0}, {"osc2level", 0.26f}, {"osc2detune", -12.0f}, {"filtercutoff", 2900.0f},
    {"attack", 1.2f}, {"decay", 1.5f}, {"sustain", 0.82f}, {"release", 2.8f},
    {"chorusenabled", 1}, {"chorusmix", 0.34f}, {"reverbenabled", 1},
    {"reverbmix", 0.3f}, {"master", 0.55f}};
constexpr ParameterSetting motionPadSettings[]{
    {"waveform", 1}, {"osc1level", 0.7f}, {"unisonvoices", 5}, {"unisondetune", 25.0f},
    {"wavetablescan", 0.22f}, {"osc2wave", 0}, {"osc2level", 0.3f}, {"lfo1rate", 0.18f},
    {"lfo1amount", 0.32f}, {"lfo1target", 2}, {"filtercutoff", 3200.0f}, {"attack", 0.8f},
    {"decay", 1.0f}, {"sustain", 0.78f}, {"release", 2.0f}, {"reverbenabled", 1},
    {"reverbmix", 0.24f}, {"master", 0.56f}};
constexpr ParameterSetting choirPadSettings[]{
    {"waveform", 1}, {"osc1level", 0.78f}, {"unisonvoices", 7}, {"unisondetune", 38.0f},
    {"osc2wave", 0}, {"osc2level", 0.2f}, {"osc3wave", 1}, {"osc3level", 0.18f},
    {"filtercutoff", 6200.0f}, {"attack", 0.65f}, {"decay", 0.8f}, {"sustain", 0.8f},
    {"release", 2.4f}, {"chorusenabled", 1}, {"chorusmix", 0.38f},
    {"reverbenabled", 1}, {"reverbmix", 0.26f}, {"master", 0.52f}};
constexpr ParameterSetting bellFxSettings[]{
    {"waveform", 0}, {"osc1level", 0.8f}, {"osc2wave", 2}, {"osc2level", 0.32f},
    {"fmamount", 0.52f}, {"filtertype", 2}, {"filtercutoff", 8500.0f},
    {"attack", 0.002f}, {"decay", 1.8f}, {"sustain", 0.0f}, {"release", 1.1f},
    {"delayenabled", 1}, {"delaymix", 0.2f}, {"reverbenabled", 1},
    {"reverbmix", 0.33f}, {"master", 0.58f}};
constexpr ParameterSetting noiseFxSettings[]{
    {"waveform", 2}, {"osc1level", 0.35f}, {"noiselevel", 0.55f}, {"filtertype", 0},
    {"filtercutoff", 3400.0f}, {"filterresonance", 0.45f}, {"filterenvamount", 0.55f},
    {"attack", 0.35f}, {"decay", 1.6f}, {"sustain", 0.48f}, {"release", 1.5f},
    {"reverbenabled", 1}, {"reverbmix", 0.4f}, {"master", 0.5f}};
constexpr ParameterSetting shortPluckSettings[]{
    {"waveform", 2}, {"osc1level", 0.8f}, {"osc2wave", 1}, {"osc2level", 0.22f},
    {"osc3wave", 3}, {"pulsewidth", 0.3f}, {"osc3level", 0.12f}, {"filtercutoff", 6200.0f},
    {"filterenvamount", 0.62f}, {"attack", 0.001f}, {"decay", 0.22f}, {"sustain", 0.08f},
    {"release", 0.16f}, {"master", 0.72f}};
constexpr ParameterSetting brightPluckSettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 2}, {"osc2level", 0.2f},
    {"ringamount", 0.22f}, {"filtertype", 2}, {"filtercutoff", 7000.0f},
    {"attack", 0.001f}, {"decay", 0.48f}, {"sustain", 0.16f}, {"release", 0.35f},
    {"delayenabled", 1}, {"delaymix", 0.12f}, {"master", 0.68f}};
constexpr ParameterSetting roundSubSettings[]{
    {"waveform", 3}, {"osc1level", 0.92f}, {"osc1octave", -1}, {"osc2level", 0.12f},
    {"sublevel", 0.72f}, {"filtercutoff", 145.0f}, {"filterresonance", 0.12f},
    {"filterenvamount", 0.2f}, {"attack", 0.004f}, {"decay", 0.22f},
    {"sustain", 0.82f}, {"release", 0.16f}, {"master", 0.68f}};
constexpr ParameterSetting acidSnapSettings[]{
    {"waveform", 2}, {"osc1level", 0.76f}, {"osc2wave", 2}, {"osc2level", 0.22f},
    {"osc2detune", -6.0f}, {"filtercutoff", 850.0f}, {"filterresonance", 0.78f},
    {"filterdrive", 0.44f}, {"filterenvamount", 0.82f}, {"filterattack", 0.004f},
    {"filterdecay", 0.38f}, {"filtersustain", 0.12f}, {"filterrelease", 0.12f},
    {"attack", 0.003f}, {"decay", 0.28f}, {"sustain", 0.32f},
    {"release", 0.14f}, {"distortionmix", 0.2f}, {"distortiondrive", 0.42f}};
constexpr ParameterSetting fmGrowlSettings[]{
    {"waveform", 0}, {"osc1level", 0.78f}, {"osc2wave", 2}, {"osc2level", 0.4f},
    {"osc2detune", -12.0f}, {"fmamount", 0.62f}, {"filtercutoff", 1050.0f},
    {"filterresonance", 0.34f}, {"filterdrive", 0.3f}, {"attack", 0.005f},
    {"decay", 0.34f}, {"sustain", 0.54f}, {"release", 0.22f},
    {"distortionmix", 0.24f}, {"distortiondrive", 0.62f}, {"master", 0.58f}};
constexpr ParameterSetting deepReeseSettings[]{
    {"waveform", 2}, {"osc1level", 0.76f}, {"unisonvoices", 3}, {"unisondetune", 14.0f},
    {"osc2wave", 2}, {"osc2level", 0.42f}, {"osc2detune", -10.0f},
    {"filtercutoff", 620.0f}, {"filterresonance", 0.42f}, {"attack", 0.012f},
    {"decay", 0.48f}, {"sustain", 0.68f}, {"release", 0.28f},
    {"chorusenabled", 1}, {"chorusmix", 0.26f}, {"stereowidth", 1.35f},
    {"master", 0.56f}};
constexpr ParameterSetting velvetEpSettings[]{
    {"waveform", 0}, {"osc1level", 0.74f}, {"osc2wave", 1}, {"osc2level", 0.24f},
    {"osc2detune", 7.0f}, {"filtercutoff", 3400.0f}, {"filterresonance", 0.12f},
    {"attack", 0.006f}, {"decay", 0.72f}, {"sustain", 0.3f}, {"release", 0.62f},
    {"reverbenabled", 1}, {"reverbmix", 0.17f}, {"master", 0.66f}};
constexpr ParameterSetting mutedClavSettings[]{
    {"waveform", 3}, {"osc1level", 0.74f}, {"osc2wave", 2}, {"osc2level", 0.18f},
    {"filtercutoff", 2450.0f}, {"filterresonance", 0.22f}, {"filterenvamount", 0.34f},
    {"attack", 0.001f}, {"decay", 0.23f}, {"sustain", 0.08f}, {"release", 0.17f},
    {"chorusenabled", 1}, {"chorusmix", 0.08f}, {"master", 0.7f}};
constexpr ParameterSetting toyBellSettings[]{
    {"waveform", 0}, {"osc1level", 0.76f}, {"osc2wave", 1}, {"osc2level", 0.24f},
    {"fmamount", 0.3f}, {"filtertype", 2}, {"filtercutoff", 5400.0f},
    {"attack", 0.002f}, {"decay", 1.0f}, {"sustain", 0.06f}, {"release", 0.92f},
    {"reverbenabled", 1}, {"reverbmix", 0.2f}, {"master", 0.62f}};
constexpr ParameterSetting glassMalletSettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 0}, {"osc2level", 0.18f},
    {"osc3wave", 1}, {"osc3level", 0.22f}, {"ringamount", 0.24f},
    {"filtertype", 2}, {"filtercutoff", 6400.0f}, {"attack", 0.002f},
    {"decay", 0.86f}, {"sustain", 0.05f}, {"release", 0.72f},
    {"reverbenabled", 1}, {"reverbmix", 0.22f}, {"master", 0.62f}};
constexpr ParameterSetting neonPulseSettings[]{
    {"waveform", 3}, {"osc1level", 0.72f}, {"osc2wave", 2}, {"osc2level", 0.3f},
    {"osc2detune", 5.0f}, {"filtercutoff", 4650.0f}, {"filterresonance", 0.36f},
    {"lfo1rate", 5.2f}, {"lfo1amount", 0.08f}, {"lfo1target", 1},
    {"attack", 0.004f}, {"decay", 0.28f}, {"sustain", 0.72f},
    {"release", 0.2f}, {"master", 0.64f}};
constexpr ParameterSetting hollowLeadSettings[]{
    {"waveform", 1}, {"osc1level", 0.72f}, {"osc2wave", 0}, {"osc2level", 0.38f},
    {"osc3wave", 0}, {"osc3level", 0.12f}, {"filtertype", 2},
    {"filtercutoff", 1650.0f}, {"filterresonance", 0.38f},
    {"lfo1rate", 0.32f}, {"lfo1amount", 0.2f}, {"lfo1target", 2},
    {"attack", 0.012f}, {"decay", 0.36f}, {"sustain", 0.65f},
    {"release", 0.28f}, {"master", 0.62f}};
constexpr ParameterSetting syncEdgeSettings[]{
    {"waveform", 3}, {"osc1level", 0.7f}, {"osc2wave", 2}, {"osc2level", 0.38f},
    {"osc2sync", 1}, {"osc2detune", 24.0f}, {"filtercutoff", 5600.0f},
    {"filterresonance", 0.38f}, {"filterdrive", 0.12f}, {"attack", 0.003f},
    {"decay", 0.32f}, {"sustain", 0.68f}, {"release", 0.22f}, {"master", 0.62f}};
constexpr ParameterSetting glideWireSettings[]{
    {"waveform", 2}, {"osc1level", 0.76f}, {"osc2wave", 0}, {"osc2level", 0.28f},
    {"osc2detune", 8.0f}, {"filtercutoff", 3800.0f}, {"filterresonance", 0.24f},
    {"voicemode", 2}, {"glideenabled", 1}, {"glidetime", 0.18f},
    {"lfo1rate", 5.8f}, {"lfo1amount", 0.06f}, {"lfo1target", 1},
    {"attack", 0.008f}, {"decay", 0.28f}, {"sustain", 0.72f},
    {"release", 0.24f}, {"master", 0.64f}};
constexpr ParameterSetting auroraPadSettings[]{
    {"waveform", 1}, {"osc1level", 0.72f}, {"unisonvoices", 7}, {"unisondetune", 28.0f},
    {"osc2wave", 0}, {"osc2level", 0.2f}, {"osc2detune", -7.0f},
    {"filtercutoff", 6200.0f}, {"attack", 1.4f}, {"decay", 1.6f},
    {"sustain", 0.82f}, {"release", 3.2f}, {"chorusenabled", 1},
    {"chorusmix", 0.32f}, {"reverbenabled", 1}, {"reverbmix", 0.28f},
    {"master", 0.52f}};
constexpr ParameterSetting tapeStringsSettings[]{
    {"waveform", 2}, {"osc1level", 0.68f}, {"unisonvoices", 5}, {"unisondetune", 22.0f},
    {"osc2wave", 1}, {"osc2level", 0.22f}, {"filtercutoff", 3700.0f},
    {"attack", 0.58f}, {"decay", 1.2f}, {"sustain", 0.74f}, {"release", 2.5f},
    {"chorusenabled", 1}, {"chorusmix", 0.34f}, {"chorusrate", 0.28f},
    {"reverbenabled", 1}, {"reverbmix", 0.2f}, {"master", 0.54f}};
constexpr ParameterSetting darkChoirSettings[]{
    {"waveform", 1}, {"osc1level", 0.68f}, {"osc2wave", 0}, {"osc2level", 0.24f},
    {"osc3wave", 0}, {"osc3level", 0.24f}, {"filtercutoff", 920.0f},
    {"filterresonance", 0.18f}, {"attack", 1.1f}, {"decay", 1.3f},
    {"sustain", 0.78f}, {"release", 3.0f}, {"chorusenabled", 1},
    {"chorusmix", 0.24f}, {"reverbenabled", 1}, {"reverbmix", 0.34f},
    {"master", 0.5f}};
constexpr ParameterSetting shimmerCloudSettings[]{
    {"waveform", 0}, {"osc1level", 0.7f}, {"unisonvoices", 7}, {"unisondetune", 40.0f},
    {"osc2wave", 1}, {"osc2level", 0.18f}, {"wavetablescan", 0.22f},
    {"filtercutoff", 7200.0f}, {"lfo1rate", 0.12f}, {"modroute1source", 0},
    {"modroute1destination", 2}, {"modroute1amount", 0.3f}, {"modroute1enabled", 1},
    {"attack", 1.8f}, {"decay", 1.4f}, {"sustain", 0.8f}, {"release", 3.5f},
    {"chorusenabled", 1}, {"chorusmix", 0.3f}, {"reverbenabled", 1},
    {"reverbmix", 0.32f}, {"master", 0.48f}};
constexpr ParameterSetting glassPinSettings[]{
    {"waveform", 0}, {"osc1level", 0.78f}, {"osc2wave", 1}, {"osc2level", 0.18f},
    {"fmamount", 0.18f}, {"filtertype", 2}, {"filtercutoff", 6800.0f},
    {"attack", 0.001f}, {"decay", 0.38f}, {"sustain", 0.1f}, {"release", 0.32f},
    {"delayenabled", 1}, {"delaymix", 0.12f}, {"master", 0.68f}};
constexpr ParameterSetting fmKalimbaSettings[]{
    {"waveform", 0}, {"osc1level", 0.74f}, {"osc2wave", 0}, {"osc2level", 0.3f},
    {"fmamount", 0.72f}, {"ringamount", 0.14f}, {"filtertype", 2},
    {"filtercutoff", 5200.0f}, {"attack", 0.001f}, {"decay", 0.66f},
    {"sustain", 0.08f}, {"release", 0.48f}, {"reverbenabled", 1},
    {"reverbmix", 0.16f}, {"master", 0.62f}};
constexpr ParameterSetting shortWireSettings[]{
    {"waveform", 3}, {"osc1level", 0.72f}, {"osc2wave", 1}, {"osc2level", 0.2f},
    {"osc3wave", 2}, {"osc3level", 0.1f}, {"filtertype", 1},
    {"filtercutoff", 460.0f}, {"filterresonance", 0.36f}, {"attack", 0.001f},
    {"decay", 0.26f}, {"sustain", 0.06f}, {"release", 0.18f}, {"master", 0.64f}};
constexpr ParameterSetting echoPluckSettings[]{
    {"waveform", 2}, {"osc1level", 0.72f}, {"osc2wave", 0}, {"osc2level", 0.2f},
    {"filtercutoff", 5200.0f}, {"attack", 0.003f}, {"decay", 0.42f},
    {"sustain", 0.12f}, {"release", 0.4f}, {"delayenabled", 1},
    {"delaymix", 0.28f}, {"delaytime", 0.31f}, {"delayfeedback", 0.38f},
    {"reverbenabled", 1}, {"reverbmix", 0.12f}, {"master", 0.6f}};
constexpr ParameterSetting laserBloomSettings[]{
    {"waveform", 0}, {"osc1level", 0.56f}, {"osc2wave", 3}, {"osc2level", 0.28f},
    {"osc3wave", 2}, {"osc3level", 0.18f}, {"fmamount", 0.44f},
    {"lfo1rate", 0.16f}, {"modroute1source", 0}, {"modroute1destination", 0},
    {"modroute1amount", 0.28f}, {"modroute1enabled", 1}, {"attack", 0.12f},
    {"decay", 0.9f}, {"sustain", 0.48f}, {"release", 1.4f},
    {"delayenabled", 1}, {"delaymix", 0.26f}, {"phaserenabled", 1},
    {"phasermix", 0.22f}, {"reverbenabled", 1}, {"reverbmix", 0.2f},
    {"master", 0.46f}};
constexpr ParameterSetting dustStormSettings[]{
    {"waveform", 2}, {"osc1level", 0.38f}, {"noiselevel", 0.6f},
    {"filtertype", 2}, {"filtercutoff", 2400.0f}, {"filterresonance", 0.46f},
    {"filterrandom", 0.3f}, {"lfo1rate", 0.08f}, {"modroute1source", 0},
    {"modroute1destination", 3}, {"modroute1amount", 0.34f}, {"modroute1enabled", 1},
    {"attack", 0.28f}, {"decay", 1.1f}, {"sustain", 0.48f}, {"release", 1.6f},
    {"reverbenabled", 1}, {"reverbmix", 0.38f}, {"master", 0.46f}};
constexpr ParameterSetting orbitBellSettings[]{
    {"waveform", 0}, {"osc1level", 0.74f}, {"osc2wave", 1}, {"osc2level", 0.26f},
    {"fmamount", 0.56f}, {"ringamount", 0.12f}, {"filtertype", 2},
    {"filtercutoff", 7800.0f}, {"lfo1rate", 0.22f}, {"modroute1source", 0},
    {"modroute1destination", 9}, {"modroute1amount", 0.2f}, {"modroute1enabled", 1},
    {"attack", 0.003f}, {"decay", 1.5f}, {"sustain", 0.05f}, {"release", 1.4f},
    {"delayenabled", 1}, {"delaymix", 0.18f}, {"reverbenabled", 1},
    {"reverbmix", 0.3f}, {"master", 0.54f}};
constexpr ParameterSetting distantSweepSettings[]{
    {"waveform", 2}, {"osc1level", 0.56f}, {"unisonvoices", 7}, {"unisondetune", 36.0f},
    {"osc2wave", 0}, {"osc2level", 0.18f}, {"noiselevel", 0.08f},
    {"filtercutoff", 1600.0f}, {"lfo1rate", 0.07f}, {"modroute1source", 0},
    {"modroute1destination", 3}, {"modroute1amount", 0.4f}, {"modroute1enabled", 1},
    {"attack", 1.5f}, {"decay", 1.2f}, {"sustain", 0.7f}, {"release", 3.2f},
    {"phaserenabled", 1}, {"phasermix", 0.24f}, {"reverbenabled", 1},
    {"reverbmix", 0.38f}, {"master", 0.44f}};
constexpr ParameterSetting windUpWaltzSettings[]{
    {"waveform", 0}, {"osc1level", 0.76f}, {"osc2wave", 1}, {"osc2level", 0.2f},
    {"fmamount", 0.18f}, {"filtertype", 2}, {"filtercutoff", 5800.0f},
    {"attack", 0.002f}, {"decay", 0.72f}, {"sustain", 0.08f}, {"release", 0.62f},
    {"reverbenabled", 1}, {"reverbmix", 0.18f}, {"master", 0.62f}};
constexpr ParameterSetting tinyCarouselSettings[]{
    {"waveform", 1}, {"osc1level", 0.72f}, {"unisonvoices", 3}, {"unisondetune", 4.0f},
    {"osc2wave", 0}, {"osc2level", 0.28f}, {"fmamount", 0.28f},
    {"filtercutoff", 7200.0f}, {"attack", 0.005f}, {"decay", 1.0f},
    {"sustain", 0.14f}, {"release", 0.92f}, {"delayenabled", 1},
    {"delaymix", 0.12f}, {"delaytime", 0.38f}, {"reverbenabled", 1},
    {"reverbmix", 0.25f}, {"master", 0.6f}};
constexpr ParameterSetting silverTeethSettings[]{
    {"waveform", 0}, {"osc1level", 0.76f}, {"osc2wave", 3}, {"osc2level", 0.16f},
    {"osc3wave", 0}, {"osc3level", 0.1f}, {"fmamount", 0.55f},
    {"ringamount", 0.1f}, {"filtertype", 2}, {"filtercutoff", 9500.0f},
    {"attack", 0.001f}, {"decay", 0.95f}, {"sustain", 0.0f}, {"release", 0.8f},
    {"reverbenabled", 1}, {"reverbmix", 0.2f}, {"master", 0.58f}};
constexpr ParameterSetting glassLullabySettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 1}, {"osc2level", 0.28f},
    {"filtertype", 2}, {"filtercutoff", 5800.0f}, {"attack", 0.04f},
    {"decay", 1.2f}, {"sustain", 0.2f}, {"release", 1.5f},
    {"reverbenabled", 1}, {"reverbmix", 0.34f}, {"master", 0.56f}};
constexpr ParameterSetting clockworkTinesSettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 1}, {"osc2level", 0.18f},
    {"osc3wave", 3}, {"osc3level", 0.14f}, {"pulsewidth", 0.23f},
    {"fmamount", 0.25f}, {"filtertype", 2}, {"filtercutoff", 7600.0f},
    {"attack", 0.001f}, {"decay", 0.52f}, {"sustain", 0.04f}, {"release", 0.44f},
    {"master", 0.64f}};
constexpr ParameterSetting velvetCarouselSettings[]{
    {"waveform", 1}, {"osc1level", 0.7f}, {"unisonvoices", 5}, {"unisondetune", 9.0f},
    {"osc2wave", 2}, {"osc2level", 0.16f}, {"filtercutoff", 4900.0f},
    {"attack", 0.12f}, {"decay", 0.86f}, {"sustain", 0.3f}, {"release", 1.1f},
    {"chorusenabled", 1}, {"chorusmix", 0.25f}, {"reverbenabled", 1},
    {"reverbmix", 0.22f}, {"master", 0.54f}};
constexpr ParameterSetting winterCrankSettings[]{
    {"waveform", 0}, {"osc1level", 0.74f}, {"osc2wave", 3}, {"osc2level", 0.12f},
    {"filtertype", 2}, {"filtercutoff", 6500.0f}, {"lfo1rate", 0.28f},
    {"lfo1amount", 0.11f}, {"lfo1target", 2}, {"attack", 0.02f},
    {"decay", 1.1f}, {"sustain", 0.16f}, {"release", 1.3f},
    {"delayenabled", 1}, {"delaymix", 0.14f}, {"reverbenabled", 1},
    {"reverbmix", 0.28f}, {"master", 0.55f}};
constexpr ParameterSetting antiqueCherubSettings[]{
    {"waveform", 0}, {"osc1level", 0.78f}, {"osc2wave", 0}, {"osc2level", 0.22f},
    {"osc2detune", 4.0f}, {"fmamount", 0.32f}, {"filtertype", 2},
    {"filtercutoff", 6200.0f}, {"attack", 0.006f}, {"decay", 0.82f},
    {"sustain", 0.1f}, {"release", 0.78f}, {"reverbenabled", 1},
    {"reverbmix", 0.24f}, {"master", 0.6f}};
constexpr ParameterSetting distantMusicBoxSettings[]{
    {"waveform", 1}, {"osc1level", 0.72f}, {"osc2wave", 0}, {"osc2level", 0.18f},
    {"filtertype", 2}, {"filtercutoff", 4200.0f}, {"filterresonance", 0.16f},
    {"attack", 0.03f}, {"decay", 1.4f}, {"sustain", 0.16f}, {"release", 1.8f},
    {"delayenabled", 1}, {"delaymix", 0.22f}, {"delaytime", 0.48f},
    {"delayfeedback", 0.42f}, {"reverbenabled", 1}, {"reverbmix", 0.3f},
    {"master", 0.5f}};
constexpr ParameterSetting brassBalletSettings[]{
    {"waveform", 2}, {"osc1level", 0.66f}, {"osc2wave", 3}, {"osc2level", 0.1f},
    {"filtercutoff", 3800.0f}, {"filterresonance", 0.24f}, {"attack", 0.018f},
    {"decay", 0.9f}, {"sustain", 0.28f}, {"release", 1.0f},
    {"chorusenabled", 1}, {"chorusmix", 0.2f}, {"reverbenabled", 1},
    {"reverbmix", 0.22f}, {"master", 0.54f}};
constexpr ParameterSetting ballerinaWaltzSettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 1}, {"osc2level", 0.22f},
    {"osc2detune", 3.0f}, {"filtertype", 2}, {"filtercutoff", 5100.0f},
    {"lfo1rate", 0.34f}, {"lfo1amount", 0.06f}, {"lfo1target", 1},
    {"attack", 0.01f}, {"decay", 0.74f}, {"sustain", 0.14f}, {"release", 0.88f},
    {"reverbenabled", 1}, {"reverbmix", 0.18f}, {"master", 0.58f}};
constexpr ParameterSetting midnightTrinketSettings[]{
    {"waveform", 0}, {"osc1level", 0.7f}, {"osc2wave", 3}, {"osc2level", 0.14f},
    {"fmamount", 0.38f}, {"filtercutoff", 1900.0f}, {"filterresonance", 0.4f},
    {"attack", 0.008f}, {"decay", 1.2f}, {"sustain", 0.1f}, {"release", 1.1f},
    {"phaserenabled", 1}, {"phasermix", 0.16f}, {"reverbenabled", 1},
    {"reverbmix", 0.28f}, {"master", 0.5f}};
constexpr ParameterSetting porcelainBellsSettings[]{
    {"waveform", 0}, {"osc1level", 0.74f}, {"osc2wave", 1}, {"osc2level", 0.24f},
    {"osc3wave", 0}, {"osc3level", 0.12f}, {"fmamount", 0.2f},
    {"filtertype", 2}, {"filtercutoff", 8400.0f}, {"attack", 0.001f},
    {"decay", 0.68f}, {"sustain", 0.02f}, {"release", 0.72f},
    {"reverbenabled", 1}, {"reverbmix", 0.21f}, {"master", 0.6f}};
constexpr ParameterSetting copperSubSettings[]{
    {"waveform", 2}, {"osc1level", 0.84f}, {"osc1octave", -1}, {"osc2wave", 0},
    {"osc2level", 0.12f}, {"sublevel", 0.82f}, {"filtercutoff", 210.0f},
    {"filterresonance", 0.2f}, {"filterenvamount", 0.34f}, {"attack", 0.003f},
    {"decay", 0.32f}, {"sustain", 0.78f}, {"release", 0.19f}, {"master", 0.66f}};
constexpr ParameterSetting feltEpSettings[]{
    {"waveform", 1}, {"osc1level", 0.78f}, {"osc2wave", 0}, {"osc2level", 0.16f},
    {"filtercutoff", 3100.0f}, {"filterenvamount", 0.18f}, {"attack", 0.014f},
    {"decay", 0.8f}, {"sustain", 0.34f}, {"release", 0.72f},
    {"chorusenabled", 1}, {"chorusmix", 0.12f}, {"master", 0.62f}};
constexpr ParameterSetting bellFluteSettings[]{
    {"waveform", 1}, {"osc1level", 0.72f}, {"osc2wave", 0}, {"osc2level", 0.22f},
    {"osc3wave", 1}, {"osc3level", 0.14f}, {"filtertype", 2},
    {"filtercutoff", 6100.0f}, {"attack", 0.04f}, {"decay", 1.0f},
    {"sustain", 0.42f}, {"release", 0.92f}, {"reverbenabled", 1},
    {"reverbmix", 0.24f}, {"master", 0.58f}};
constexpr ParameterSetting octaveSparkSettings[]{
    {"waveform", 3}, {"osc1level", 0.68f}, {"osc1octave", 1}, {"osc2wave", 2},
    {"osc2level", 0.28f}, {"osc2detune", -5.0f}, {"filtercutoff", 5100.0f},
    {"filterresonance", 0.32f}, {"attack", 0.004f}, {"decay", 0.28f},
    {"sustain", 0.66f}, {"release", 0.24f}, {"master", 0.58f}};
constexpr ParameterSetting sirenRibbonSettings[]{
    {"waveform", 2}, {"osc1level", 0.72f}, {"unisonvoices", 3}, {"unisondetune", 8.0f},
    {"osc2wave", 1}, {"osc2level", 0.24f}, {"filtercutoff", 1800.0f},
    {"filterresonance", 0.44f}, {"lfo1rate", 0.62f}, {"lfo1amount", 0.24f},
    {"lfo1target", 2}, {"voicemode", 2}, {"glideenabled", 1}, {"glidetime", 0.28f},
    {"attack", 0.06f}, {"decay", 0.4f}, {"sustain", 0.7f},
    {"release", 0.42f}, {"master", 0.56f}};
constexpr ParameterSetting haloPadSettings[]{
    {"waveform", 1}, {"osc1level", 0.68f}, {"unisonvoices", 7}, {"unisondetune", 31.0f},
    {"osc2wave", 0}, {"osc2level", 0.2f}, {"filtercutoff", 4800.0f},
    {"lfo1rate", 0.16f}, {"modroute1source", 0}, {"modroute1destination", 8},
    {"modroute1amount", 0.24f}, {"modroute1enabled", 1}, {"attack", 1.3f},
    {"decay", 1.1f}, {"sustain", 0.76f}, {"release", 3.0f},
    {"chorusenabled", 1}, {"chorusmix", 0.3f}, {"reverbenabled", 1},
    {"reverbmix", 0.34f}, {"master", 0.48f}};
constexpr ParameterSetting paperMoonSettings[]{
    {"waveform", 0}, {"osc1level", 0.72f}, {"osc2wave", 1}, {"osc2level", 0.16f},
    {"osc2detune", 5.0f}, {"filtercutoff", 2600.0f}, {"lfo1rate", 0.09f},
    {"lfo1amount", 0.12f}, {"lfo1target", 2}, {"attack", 0.9f},
    {"decay", 1.4f}, {"sustain", 0.72f}, {"release", 2.7f},
    {"reverbenabled", 1}, {"reverbmix", 0.32f}, {"master", 0.52f}};
constexpr ParameterSetting woodenKalimbaSettings[]{
    {"waveform", 0}, {"osc1level", 0.74f}, {"osc2wave", 1}, {"osc2level", 0.18f},
    {"osc3wave", 2}, {"osc3level", 0.1f}, {"fmamount", 0.3f},
    {"filtertype", 2}, {"filtercutoff", 4300.0f}, {"attack", 0.001f},
    {"decay", 0.54f}, {"sustain", 0.04f}, {"release", 0.32f},
    {"reverbenabled", 1}, {"reverbmix", 0.12f}, {"master", 0.62f}};
constexpr ParameterSetting chimePluckSettings[]{
    {"waveform", 0}, {"osc1level", 0.76f}, {"osc2wave", 1}, {"osc2level", 0.2f},
    {"fmamount", 0.42f}, {"filtertype", 2}, {"filtercutoff", 7600.0f},
    {"attack", 0.001f}, {"decay", 0.62f}, {"sustain", 0.06f}, {"release", 0.5f},
    {"delayenabled", 1}, {"delaymix", 0.16f}, {"delaytime", 0.24f},
    {"reverbenabled", 1}, {"reverbmix", 0.18f}, {"master", 0.6f}};
constexpr ParameterSetting radioDustSettings[]{
    {"waveform", 2}, {"osc1level", 0.32f}, {"noiselevel", 0.54f},
    {"filtertype", 0}, {"filtercutoff", 2850.0f}, {"filterresonance", 0.36f},
    {"filterrandom", 0.24f}, {"lfo1rate", 0.12f}, {"modroute1source", 0},
    {"modroute1destination", 3}, {"modroute1amount", 0.24f}, {"modroute1enabled", 1},
    {"attack", 0.18f}, {"decay", 1.0f}, {"sustain", 0.38f}, {"release", 1.2f},
    {"delayenabled", 1}, {"delaymix", 0.14f}, {"reverbenabled", 1},
    {"reverbmix", 0.26f}, {"master", 0.48f}};
constexpr ParameterSetting orbitSweepSettings[]{
    {"waveform", 2}, {"osc1level", 0.54f}, {"unisonvoices", 5}, {"unisondetune", 26.0f},
    {"osc2wave", 0}, {"osc2level", 0.24f}, {"filtercutoff", 1200.0f},
    {"lfo1rate", 0.11f}, {"modroute1source", 0}, {"modroute1destination", 3},
    {"modroute1amount", 0.36f}, {"modroute1enabled", 1}, {"attack", 0.72f},
    {"decay", 1.3f}, {"sustain", 0.6f}, {"release", 2.6f},
    {"phaserenabled", 1}, {"phasermix", 0.26f}, {"reverbenabled", 1},
    {"reverbmix", 0.3f}, {"master", 0.46f}};

const BuiltInPreset builtInPresets[]{
    {"Sub Bass", "Bass", subBassSettings, std::size(subBassSettings)},
    {"Reese Bass", "Bass", reeseBassSettings, std::size(reeseBassSettings)},
    {"Pluck Bass", "Bass", pluckBassSettings, std::size(pluckBassSettings)},
    {"Warm Keys", "Keys", warmKeysSettings, std::size(warmKeysSettings)},
    {"Glass Keys", "Keys", glassKeysSettings, std::size(glassKeysSettings)},
    {"Soft Piano", "Keys", softPianoSettings, std::size(softPianoSettings)},
    {"Mono Lead", "Lead", monoLeadSettings, std::size(monoLeadSettings)},
    {"Sync Lead", "Lead", syncLeadSettings, std::size(syncLeadSettings)},
    {"FM Lead", "Lead", fmLeadSettings, std::size(fmLeadSettings)},
    {"Slow Pad", "Pad", slowPadSettings, std::size(slowPadSettings)},
    {"Motion Pad", "Pad", motionPadSettings, std::size(motionPadSettings)},
    {"Choir Pad", "Pad", choirPadSettings, std::size(choirPadSettings)},
    {"Bell FX", "FX", bellFxSettings, std::size(bellFxSettings)},
    {"Noise FX", "FX", noiseFxSettings, std::size(noiseFxSettings)},
    {"Short Pluck", "Pluck", shortPluckSettings, std::size(shortPluckSettings)},
    {"Bright Pluck", "Pluck", brightPluckSettings, std::size(brightPluckSettings)},
    {"Round Sub", "Bass", roundSubSettings, std::size(roundSubSettings)},
    {"Acid Snap", "Bass", acidSnapSettings, std::size(acidSnapSettings)},
    {"FM Growl", "Bass", fmGrowlSettings, std::size(fmGrowlSettings)},
    {"Deep Reese", "Bass", deepReeseSettings, std::size(deepReeseSettings)},
    {"Velvet EP", "Keys", velvetEpSettings, std::size(velvetEpSettings)},
    {"Muted Clav", "Keys", mutedClavSettings, std::size(mutedClavSettings)},
    {"Toy Bell", "Keys", toyBellSettings, std::size(toyBellSettings)},
    {"Glass Mallet", "Keys", glassMalletSettings, std::size(glassMalletSettings)},
    {"Neon Pulse", "Lead", neonPulseSettings, std::size(neonPulseSettings)},
    {"Hollow Lead", "Lead", hollowLeadSettings, std::size(hollowLeadSettings)},
    {"Sync Edge", "Lead", syncEdgeSettings, std::size(syncEdgeSettings)},
    {"Glide Wire", "Lead", glideWireSettings, std::size(glideWireSettings)},
    {"Aurora", "Pad", auroraPadSettings, std::size(auroraPadSettings)},
    {"Tape Strings", "Pad", tapeStringsSettings, std::size(tapeStringsSettings)},
    {"Dark Choir", "Pad", darkChoirSettings, std::size(darkChoirSettings)},
    {"Shimmer Cloud", "Pad", shimmerCloudSettings, std::size(shimmerCloudSettings)},
    {"Glass Pin", "Pluck", glassPinSettings, std::size(glassPinSettings)},
    {"FM Kalimba", "Pluck", fmKalimbaSettings, std::size(fmKalimbaSettings)},
    {"Short Wire", "Pluck", shortWireSettings, std::size(shortWireSettings)},
    {"Echo Pluck", "Pluck", echoPluckSettings, std::size(echoPluckSettings)},
    {"Laser Bloom", "FX", laserBloomSettings, std::size(laserBloomSettings)},
    {"Dust Storm", "FX", dustStormSettings, std::size(dustStormSettings)},
    {"Orbit Bell", "FX", orbitBellSettings, std::size(orbitBellSettings)},
    {"Distant Sweep", "FX", distantSweepSettings, std::size(distantSweepSettings)},
    {"Wind-Up Waltz", "Music Box", windUpWaltzSettings, std::size(windUpWaltzSettings)},
    {"Tiny Carousel", "Music Box", tinyCarouselSettings, std::size(tinyCarouselSettings)},
    {"Silver Teeth", "Music Box", silverTeethSettings, std::size(silverTeethSettings)},
    {"Glass Lullaby", "Music Box", glassLullabySettings, std::size(glassLullabySettings)},
    {"Clockwork Tines", "Music Box", clockworkTinesSettings, std::size(clockworkTinesSettings)},
    {"Velvet Carousel", "Music Box", velvetCarouselSettings, std::size(velvetCarouselSettings)},
    {"Winter Crank", "Music Box", winterCrankSettings, std::size(winterCrankSettings)},
    {"Antique Cherub", "Music Box", antiqueCherubSettings, std::size(antiqueCherubSettings)},
    {"Distant Music Box", "Music Box", distantMusicBoxSettings, std::size(distantMusicBoxSettings)},
    {"Brass Ballet", "Music Box", brassBalletSettings, std::size(brassBalletSettings)},
    {"Ballerina Waltz", "Music Box", ballerinaWaltzSettings, std::size(ballerinaWaltzSettings)},
    {"Midnight Trinket", "Music Box", midnightTrinketSettings, std::size(midnightTrinketSettings)},
    {"Porcelain Bells", "Music Box", porcelainBellsSettings, std::size(porcelainBellsSettings)},
    {"Copper Sub", "Bass", copperSubSettings, std::size(copperSubSettings)},
    {"Felt EP", "Keys", feltEpSettings, std::size(feltEpSettings)},
    {"Bell Flute", "Keys", bellFluteSettings, std::size(bellFluteSettings)},
    {"Octave Spark", "Lead", octaveSparkSettings, std::size(octaveSparkSettings)},
    {"Siren Ribbon", "Lead", sirenRibbonSettings, std::size(sirenRibbonSettings)},
    {"Halo Pad", "Pad", haloPadSettings, std::size(haloPadSettings)},
    {"Paper Moon", "Pad", paperMoonSettings, std::size(paperMoonSettings)},
    {"Wooden Kalimba", "Pluck", woodenKalimbaSettings, std::size(woodenKalimbaSettings)},
    {"Chime Pluck", "Pluck", chimePluckSettings, std::size(chimePluckSettings)},
    {"Radio Dust", "FX", radioDustSettings, std::size(radioDustSettings)},
    {"Orbit Sweep", "FX", orbitSweepSettings, std::size(orbitSweepSettings)}
};

juce::String getDocumentText(const juce::var& document, const juce::Identifier& key) {
    return document.getProperty(key, {}).toString();
}
} // namespace

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& parameterState)
    : parameters_(parameterState) {}

void PresetManager::setStateLoadedCallback(std::function<void()> callback) {
    stateLoadedCallback_ = std::move(callback);
}

int PresetManager::getBuiltInPresetCount() noexcept {
    return static_cast<int>(std::size(builtInPresets));
}

PresetManager::PresetInfo PresetManager::getBuiltInPresetInfo(int index) {
    if (index < 0 || index >= getBuiltInPresetCount())
        return {};
    const auto& preset = builtInPresets[index];
    return {preset.name, preset.category, true, {}};
}

std::vector<std::pair<juce::String, float>> PresetManager::getBuiltInPresetValues(int index) {
    std::vector<std::pair<juce::String, float>> result;
    if (index < 0 || index >= getBuiltInPresetCount())
        return result;
    const auto& preset = builtInPresets[index];
    result.reserve(preset.settingCount);
    for (std::size_t settingIndex = 0; settingIndex < preset.settingCount; ++settingIndex) {
        const auto& setting = preset.settings[settingIndex];
        result.emplace_back(setting.id, setting.value);
    }
    return result;
}

juce::String PresetManager::builtInPresetId(int index) {
    return index >= 0 && index < getBuiltInPresetCount()
               ? "builtin:" + juce::String(index)
               : juce::String{};
}

juce::String PresetManager::userPresetId(const juce::File& file) {
    return file.getFileName().isNotEmpty() ? "user:" + file.getFileName() : juce::String{};
}

bool PresetManager::loadBuiltInPreset(int index) {
    if (index < 0 || index >= getBuiltInPresetCount())
        return false;

    const auto currentState = parameters_.copyState();
    for (int child = 0; child < currentState.getNumChildren(); ++child) {
        const auto parameterNode = currentState.getChild(child);
        if (!parameterNode.hasProperty("id"))
            continue;
        if (auto* parameter = parameters_.getParameter(parameterNode.getProperty("id").toString()))
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
    }

    const auto& preset = builtInPresets[index];
    for (std::size_t settingIndex = 0; settingIndex < preset.settingCount; ++settingIndex) {
        const auto& setting = preset.settings[settingIndex];
        if (auto* parameter = parameters_.getParameter(setting.id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(setting.value));
    }

    // Factory presets contain no Morph Matrix state. Remove any prior user-captured
    // corners/locks so they cannot keep overriding the newly loaded parameter values.
    for (int index = parameters_.state.getNumProperties() - 1; index >= 0; --index) {
        const auto property = parameters_.state.getPropertyName(index);
        if (property.toString().startsWith("morph"))
            parameters_.state.removeProperty(property, nullptr);
    }
    if (stateLoadedCallback_)
        stateLoadedCallback_();
    return true;
}

juce::File PresetManager::getUserPresetDirectory() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("PAS Audio")
        .getChildFile("PAS 1")
        .getChildFile("Presets");
}

bool PresetManager::isUserPresetFile(const juce::File& file) const {
    return file.existsAsFile()
           && file.getFileExtension().equalsIgnoreCase(".pas1preset")
           && file.getParentDirectory() == getUserPresetDirectory();
}

std::vector<PresetManager::PresetInfo> PresetManager::getUserPresets() const {
    std::vector<PresetInfo> presets;
    const auto directory = getUserPresetDirectory();
    if (!directory.isDirectory())
        return presets;

    juce::Array<juce::File> files;
    directory.findChildFiles(files, juce::File::findFiles, false, "*.pas1preset");
    files.sort();
    presets.reserve(static_cast<std::size_t>(files.size()));
    for (const auto& file : files) {
        juce::String name;
        juce::String category;
        juce::ValueTree unusedState;
        juce::String error;
        if (readPresetDocument(file, unusedState, name, category, error))
            presets.push_back({name, category.isNotEmpty() ? category : "User", false, file});
    }
    return presets;
}

juce::File PresetManager::getLibraryMetadataFile() const {
    return getUserPresetDirectory().getParentDirectory().getChildFile("preset-library.json");
}

juce::StringArray PresetManager::getLibraryArray(const juce::Identifier& property) const {
    const auto file = getLibraryMetadataFile();
    if (!file.existsAsFile() || file.getSize() > 64 * 1024)
        return {};
    const auto document = juce::JSON::parse(file.loadFileAsString());
    if (!document.isObject()
        || getDocumentText(document, "format") != "PAS1Library"
        || static_cast<int>(document.getProperty("version", 0)) != 1)
        return {};
    const auto values = document.getProperty(property, {});
    const auto* array = values.getArray();
    if (array == nullptr)
        return {};
    juce::StringArray result;
    for (const auto& value : *array) {
        const auto id = value.toString();
        if (id.isNotEmpty() && !result.contains(id))
            result.add(id);
    }
    return result;
}

juce::StringArray PresetManager::getFavoritePresetIds() const {
    return getLibraryArray("favorites");
}

juce::StringArray PresetManager::getRecentPresetIds() const {
    return getLibraryArray("recent");
}

bool PresetManager::writeLibraryMetadata(const juce::StringArray& favorites,
                                         const juce::StringArray& recent) const {
    const auto directory = getUserPresetDirectory().getParentDirectory();
    if (directory.createDirectory().failed())
        return false;
    auto favoritesArray = juce::Array<juce::var>{};
    auto recentArray = juce::Array<juce::var>{};
    for (const auto& id : favorites)
        favoritesArray.add(id);
    for (const auto& id : recent)
        recentArray.add(id);
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("format", "PAS1Library");
    object->setProperty("version", 1);
    object->setProperty("favorites", juce::var(favoritesArray));
    object->setProperty("recent", juce::var(recentArray));
    return getLibraryMetadataFile().replaceWithText(
        juce::JSON::toString(juce::var(object.release()), true));
}

bool PresetManager::setPresetFavorite(const juce::String& id, bool favorite) const {
    if (id.isEmpty())
        return false;
    auto favorites = getFavoritePresetIds();
    favorites.removeString(id);
    if (favorite)
        favorites.add(id);
    return writeLibraryMetadata(favorites, getRecentPresetIds());
}

void PresetManager::recordRecentPreset(const juce::String& id) const {
    if (id.isEmpty())
        return;
    juce::StringArray recent;
    recent.add(id);
    for (const auto& existing : getRecentPresetIds()) {
        if (existing != id && recent.size() < 20)
            recent.add(existing);
    }
    writeLibraryMetadata(getFavoritePresetIds(), recent);
}

bool PresetManager::saveUserPreset(const juce::String& name, const juce::String& category,
                                   juce::File& savedFile, juce::String& error) const {
    auto directory = getUserPresetDirectory();
    if (const auto result = directory.createDirectory(); result.failed()) {
        error = result.getErrorMessage();
        return false;
    }

    auto fileName = juce::File::createLegalFileName(name.trim());
    if (fileName.isEmpty()) {
        error = "Enter a preset name.";
        return false;
    }
    auto file = directory.getChildFile(fileName + ".pas1preset");
    while (file.existsAsFile())
        file = file.getNonexistentSibling();
    const auto presetCategory = category.trim().isEmpty() ? juce::String{"User"} : category.trim();
    if (!writePresetFile(file, name.trim(), presetCategory, error))
        return false;

    savedFile = file;
    return true;
}

bool PresetManager::overwriteUserPreset(const juce::File& file, juce::String& error) const {
    if (!isUserPresetFile(file)) {
        error = "Only presets in the PAS 1 user library can be updated.";
        return false;
    }
    juce::ValueTree unusedState;
    juce::String name;
    juce::String category;
    if (!readPresetDocument(file, unusedState, name, category, error))
        return false;
    return writePresetFile(file, name, category, error);
}

bool PresetManager::duplicateBuiltInPreset(int index, const juce::String& newName,
                                           juce::File& duplicateFile, juce::String& error) const {
    if (index < 0 || index >= getBuiltInPresetCount()) {
        error = "The selected built-in preset no longer exists.";
        return false;
    }
    const auto cleanName = newName.trim();
    const auto legalName = juce::File::createLegalFileName(cleanName);
    if (cleanName.isEmpty() || legalName.isEmpty()) {
        error = "Enter a valid name for the duplicate.";
        return false;
    }

    auto state = parameters_.copyState();
    for (int child = 0; child < state.getNumChildren(); ++child) {
        auto node = state.getChild(child);
        if (!node.hasProperty("id"))
            continue;
        if (const auto* parameter = parameters_.getParameter(node.getProperty("id").toString()))
            node.setProperty("value",
                             parameter->convertFrom0to1(parameter->getDefaultValue()), nullptr);
    }
    const auto& preset = builtInPresets[index];
    for (std::size_t item = 0; item < preset.settingCount; ++item) {
        const auto& setting = preset.settings[item];
        for (int child = 0; child < state.getNumChildren(); ++child) {
            auto node = state.getChild(child);
            if (node.getProperty("id").toString() == setting.id) {
                node.setProperty("value", setting.value, nullptr);
                break;
            }
        }
    }
    auto target = getUserPresetDirectory().getChildFile(legalName + ".pas1preset");
    if (const auto result = target.getParentDirectory().createDirectory(); result.failed()) {
        error = result.getErrorMessage();
        return false;
    }
    while (target.existsAsFile())
        target = target.getNonexistentSibling();
    if (!writePresetFileFromState(target, cleanName, preset.category, state, error))
        return false;
    duplicateFile = target;
    return true;
}

bool PresetManager::duplicateUserPreset(const juce::File& source, const juce::String& newName,
                                        juce::File& duplicateFile, juce::String& error) const {
    if (!isUserPresetFile(source)) {
        error = "Only presets in the PAS 1 user library can be duplicated here.";
        return false;
    }
    juce::ValueTree state;
    juce::String originalName;
    juce::String category;
    if (!readPresetDocument(source, state, originalName, category, error))
        return false;
    const auto cleanName = newName.trim();
    const auto legalName = juce::File::createLegalFileName(cleanName);
    if (cleanName.isEmpty() || legalName.isEmpty()) {
        error = "Enter a valid name for the duplicate.";
        return false;
    }
    auto target = getUserPresetDirectory().getChildFile(legalName + ".pas1preset");
    while (target.existsAsFile())
        target = target.getNonexistentSibling();
    if (!writePresetFileFromState(target, cleanName, category, state, error))
        return false;
    duplicateFile = target;
    return true;
}

bool PresetManager::renameUserPreset(const juce::File& source, const juce::String& newName,
                                     juce::File& renamedFile, juce::String& error) const {
    if (!isUserPresetFile(source)) {
        error = "Only presets in the PAS 1 user library can be renamed.";
        return false;
    }
    juce::ValueTree state;
    juce::String oldName;
    juce::String category;
    if (!readPresetDocument(source, state, oldName, category, error))
        return false;
    const auto cleanName = newName.trim();
    const auto legalName = juce::File::createLegalFileName(cleanName);
    if (cleanName.isEmpty() || legalName.isEmpty()) {
        error = "Enter a valid preset name.";
        return false;
    }
    const auto target = getUserPresetDirectory().getChildFile(legalName + ".pas1preset");
    if (target != source && target.existsAsFile()) {
        error = "A user preset with that file name already exists.";
        return false;
    }
    if (!writePresetFileFromState(target, cleanName, category, state, error))
        return false;
    if (target != source && !source.deleteFile()) {
        error = "The renamed preset was written, but the old file could not be removed.";
        return false;
    }

    const auto oldId = userPresetId(source);
    const auto newId = userPresetId(target);
    auto favorites = getFavoritePresetIds();
    auto recent = getRecentPresetIds();
    for (int index = 0; index < favorites.size(); ++index)
        if (favorites[index] == oldId)
            favorites.set(index, newId);
    for (int index = 0; index < recent.size(); ++index)
        if (recent[index] == oldId)
            recent.set(index, newId);
    writeLibraryMetadata(favorites, recent);
    renamedFile = target;
    return true;
}

bool PresetManager::deleteUserPreset(const juce::File& file, juce::String& error) const {
    if (!isUserPresetFile(file)) {
        error = "Only presets in the PAS 1 user library can be deleted.";
        return false;
    }
    const auto id = userPresetId(file);
    if (!file.deleteFile()) {
        error = "Could not delete the selected preset file.";
        return false;
    }
    auto favorites = getFavoritePresetIds();
    auto recent = getRecentPresetIds();
    favorites.removeString(id);
    recent.removeString(id);
    writeLibraryMetadata(favorites, recent);
    return true;
}

bool PresetManager::exportPreset(const juce::File& destination, const juce::String& name,
                                 const juce::String& category, juce::String& error) const {
    if (destination == juce::File{})
        return false;
    const auto presetCategory = category.trim().isEmpty() ? juce::String{"Export"} : category.trim();
    return writePresetFile(destination, name.trim(), presetCategory, error);
}

bool PresetManager::exportUserPreset(const juce::File& source, const juce::File& destination,
                                     juce::String& error) const {
    if (!isUserPresetFile(source)) {
        error = "Only presets from the PAS 1 user library can be exported directly.";
        return false;
    }
    if (destination == juce::File{} || destination == source) {
        error = "Choose a different destination file.";
        return false;
    }
    juce::ValueTree unusedState;
    juce::String name;
    juce::String category;
    if (!readPresetDocument(source, unusedState, name, category, error))
        return false;
    if (!source.copyFileTo(destination)) {
        error = "Could not export the selected preset.";
        return false;
    }
    return true;
}

bool PresetManager::loadPreset(const juce::File& file, juce::String& error) {
    juce::ValueTree state;
    juce::String name;
    juce::String category;
    if (!readPresetDocument(file, state, name, category, error))
        return false;

    parameters_.replaceState(state);
    if (stateLoadedCallback_)
        stateLoadedCallback_();
    return true;
}

bool PresetManager::importPreset(const juce::File& source, juce::File& importedFile,
                                 juce::String& error) const {
    juce::ValueTree unusedState;
    juce::String name;
    juce::String category;
    if (!readPresetDocument(source, unusedState, name, category, error))
        return false;

    const auto directory = getUserPresetDirectory();
    if (const auto result = directory.createDirectory(); result.failed()) {
        error = result.getErrorMessage();
        return false;
    }

    auto target = directory.getChildFile(
        juce::File::createLegalFileName(name) + ".pas1preset");
    while (target.existsAsFile())
        target = target.getNonexistentSibling();
    if (!source.copyFileTo(target)) {
        error = "Could not copy the preset into the user preset folder.";
        return false;
    }
    importedFile = target;
    return true;
}

bool PresetManager::makePresetDocument(const juce::String& name, const juce::String& category,
                                       juce::String& document, juce::String& error) const {
    return makePresetDocumentFromState(parameters_.copyState(), name, category, document, error);
}

bool PresetManager::makePresetDocumentFromState(const juce::ValueTree& sourceState,
                                               const juce::String& name,
                                               const juce::String& category,
                                               juce::String& document,
                                               juce::String& error) const {
    auto state = sourceState.createCopy();
    state.setProperty("stateVersion", 3, nullptr);
    const auto xml = state.createXml();
    if (xml == nullptr) {
        error = "Could not serialize the current synthesizer state.";
        return false;
    }

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("format", "PAS1Preset");
    object->setProperty("schemaVersion", 1);
    object->setProperty("name", name);
    object->setProperty("category", category);
    object->setProperty("stateXml", xml->toString());
    document = juce::JSON::toString(juce::var(object.release()), true);
    return true;
}

bool PresetManager::readPresetDocument(const juce::File& file, juce::ValueTree& state,
                                       juce::String& name, juce::String& category,
                                       juce::String& error) const {
    if (!file.existsAsFile() || file.getSize() > 1024 * 1024) {
        error = "Preset file is missing or larger than 1 MB.";
        return false;
    }

    const auto document = juce::JSON::parse(file.loadFileAsString());
    if (!document.isObject()
        || getDocumentText(document, "format") != "PAS1Preset"
        || static_cast<int>(document.getProperty("schemaVersion", 0)) != 1) {
        error = "This is not a supported PAS 1 preset file.";
        return false;
    }

    name = getDocumentText(document, "name");
    category = getDocumentText(document, "category");
    if (name.isEmpty()) {
        error = "Preset file has no name.";
        return false;
    }
    return validateStateXml(getDocumentText(document, "stateXml"), state, error);
}

bool PresetManager::validateStateXml(const juce::String& xmlText, juce::ValueTree& state,
                                     juce::String& error) const {
    const auto xml = juce::XmlDocument::parse(xmlText);
    if (xml == nullptr || !xml->hasTagName(parameters_.state.getType())) {
        error = "Preset contains invalid or incompatible synthesizer state.";
        return false;
    }

    auto candidate = juce::ValueTree::fromXml(*xml);
    if (!candidate.isValid()) {
        error = "Preset state could not be read.";
        return false;
    }
    for (int index = 0; index < candidate.getNumChildren(); ++index) {
        const auto parameterNode = candidate.getChild(index);
        if (!parameterNode.hasProperty("id") || !parameterNode.hasProperty("value"))
            continue;
        const auto parameterId = parameterNode.getProperty("id").toString();
        if (parameters_.getParameter(parameterId) == nullptr)
            continue;
        const auto* parameter = parameters_.getParameter(parameterId);
        if (parameter == nullptr)
            continue;
        const auto value = static_cast<float>(parameterNode.getProperty("value"));
        const auto& range = parameter->getNormalisableRange();
        if (!std::isfinite(value) || value < range.start - 1.0e-5f
            || value > range.end + 1.0e-5f) {
            error = "Preset contains a parameter value outside its valid range.";
            return false;
        }
    }
    state = candidate;
    return true;
}

bool PresetManager::writePresetFile(const juce::File& destination, const juce::String& name,
                                    const juce::String& category, juce::String& error) const {
    return writePresetFileFromState(destination, name, category, parameters_.copyState(), error);
}

bool PresetManager::writePresetFileFromState(const juce::File& destination,
                                             const juce::String& name,
                                             const juce::String& category,
                                             const juce::ValueTree& state,
                                             juce::String& error) const {
    juce::String document;
    if (name.isEmpty() || !makePresetDocumentFromState(state, name, category, document, error)) {
        if (error.isEmpty())
            error = "Enter a preset name.";
        return false;
    }

    if (!destination.replaceWithText(document)) {
        error = "Could not write the preset file.";
        return false;
    }
    return true;
}

