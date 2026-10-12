#include "SoundDNA.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace pas1::dsp {
namespace {
float unit(float value) noexcept {
    return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 1.0f);
}

float parameter(const std::vector<pas1::generation::ParameterValue>& parameters,
                std::string_view id, float fallback = 0.0f) {
    const auto found = std::find_if(parameters.begin(), parameters.end(),
                                    [id](const auto& candidate) {
                                        return std::string_view{candidate.id} == id;
                                    });
    return found == parameters.end() ? fallback : unit(found->normalizedValue);
}
}

float SoundDNAProfile::get(SoundDNAFeature feature) const noexcept {
    const auto index = static_cast<std::size_t>(feature);
    return index < values.size() ? values[index] : 0.0f;
}

SoundDNAProfile SoundDNA::analyze(
    const std::vector<pas1::generation::ParameterValue>& parameters) {
    const auto cutoff = parameter(parameters, "filtercutoff", 0.5f);
    const auto scan = parameter(parameters, "wavetablescan");
    const auto osc1 = parameter(parameters, "osc1level");
    const auto osc2 = parameter(parameters, "osc2level");
    const auto osc3 = parameter(parameters, "osc3level");
    const auto unison = parameter(parameters, "unisonvoices");
    const auto detune = parameter(parameters, "unisondetune");
    const auto resonance = parameter(parameters, "filterresonance");
    const auto fm = parameter(parameters, "fmamount");
    const auto ring = parameter(parameters, "ringamount");
    const auto noise = parameter(parameters, "noiselevel");
    const auto sub = parameter(parameters, "sublevel");
    const auto lfo1 = parameter(parameters, "lfo1amount");
    const auto lfo2 = parameter(parameters, "lfo2amount");
    const auto chorus = parameter(parameters, "chorusmix")
                        * parameter(parameters, "chorusenabled", 1.0f);
    const auto phaser = parameter(parameters, "phasermix")
                        * parameter(parameters, "phaserenabled", 1.0f);
    const auto delay = parameter(parameters, "delaymix")
                       * parameter(parameters, "delayenabled", 1.0f);
    const auto reverb = parameter(parameters, "reverbmix")
                        * parameter(parameters, "reverbenabled", 1.0f);
    const auto reverbDecay = parameter(parameters, "reverbdecay");
    const auto distortion = parameter(parameters, "distortionmix")
                            * parameter(parameters, "distortionenabled", 1.0f);
    const auto filterDrive = parameter(parameters, "filterdrive");
    const auto stereoWidth = parameter(parameters, "stereowidth", 0.5f);
    const auto attack = parameter(parameters, "attack", 0.2f);
    const auto decay = parameter(parameters, "decay", 0.3f);
    const auto sustain = parameter(parameters, "sustain", 0.7f);
    const auto release = parameter(parameters, "release", 0.25f);
    const auto oscillatorShape = parameter(parameters, "waveform", 0.66f);

    SoundDNAProfile result;
    const auto eqHigh = parameter(parameters, "eqhighgain", 0.5f);
    const auto eqHighContribution = parameter(parameters, "eqenabled", 1.0f)
                                        >= 0.5f
                                        ? eqHigh : 0.5f;
    result.values[static_cast<std::size_t>(SoundDNAFeature::brightness)] =
        unit(0.57f * cutoff + 0.17f * scan + 0.12f * oscillatorShape
             + 0.14f * eqHighContribution);
    result.values[static_cast<std::size_t>(SoundDNAFeature::harmonicComplexity)] =
        unit(0.27f * fm + 0.17f * ring + 0.16f * scan + 0.14f * osc2
             + 0.14f * osc3 + 0.12f * noise);
    result.values[static_cast<std::size_t>(SoundDNAFeature::density)] =
        unit(0.28f * unison + 0.18f * detune + 0.16f * osc2 + 0.12f * osc3
             + 0.14f * sub + 0.12f * noise);
    result.values[static_cast<std::size_t>(SoundDNAFeature::movement)] =
        unit(0.25f * lfo1 + 0.1f * lfo2 + 0.22f * scan + 0.15f * chorus
             + 0.12f * phaser + 0.08f * delay + 0.08f * reverb);
    result.values[static_cast<std::size_t>(SoundDNAFeature::aggression)] =
        unit(0.32f * distortion + 0.24f * filterDrive + 0.18f * resonance
             + 0.14f * noise + 0.12f * fm);
    result.values[static_cast<std::size_t>(SoundDNAFeature::stereoWidth)] =
        unit(0.78f * stereoWidth + 0.22f * chorus);
    result.values[static_cast<std::size_t>(SoundDNAFeature::bassWeight)] =
        unit(0.37f * sub + 0.24f * (1.0f - cutoff) + 0.23f * osc1
             + 0.1f * osc2 + 0.06f * (1.0f - oscillatorShape));
    result.values[static_cast<std::size_t>(SoundDNAFeature::space)] =
        unit(0.42f * reverb + 0.2f * delay + 0.18f * reverbDecay
             + 0.12f * chorus + 0.08f * phaser);
    result.values[static_cast<std::size_t>(SoundDNAFeature::texture)] =
        unit(0.25f * noise + 0.22f * fm + 0.18f * ring + 0.17f * scan
             + 0.1f * phaser + 0.08f * filterDrive);
    result.values[static_cast<std::size_t>(SoundDNAFeature::envelopeCharacter)] =
        unit(0.3f * attack + 0.18f * decay + 0.27f * sustain + 0.25f * release);
    return result;
}

float SoundDNA::distance(const SoundDNAProfile& first,
                         const SoundDNAProfile& second) noexcept {
    double squared = 0.0;
    for (std::size_t index = 0; index < SoundDNAProfile::featureCount; ++index) {
        const auto delta = static_cast<double>(unit(first.values[index]))
                           - static_cast<double>(unit(second.values[index]));
        squared += delta * delta;
    }
    return static_cast<float>(std::sqrt(squared / SoundDNAProfile::featureCount));
}

SoundDNAProfile SoundDNA::blend(const SoundDNAProfile& first,
                                const SoundDNAProfile& second, float amount) noexcept {
    amount = unit(amount);
    SoundDNAProfile result;
    for (std::size_t index = 0; index < result.values.size(); ++index)
        result.values[index] = unit(first.values[index] * (1.0f - amount)
                                    + second.values[index] * amount);
    return result;
}

pas1::generation::RecipeControls SoundDNA::toRecipeControls(
    const SoundDNAProfile& profile) noexcept {
    return {profile.get(SoundDNAFeature::brightness),
            profile.get(SoundDNAFeature::brightness),
            profile.get(SoundDNAFeature::density),
            profile.get(SoundDNAFeature::movement),
            profile.get(SoundDNAFeature::aggression),
            profile.get(SoundDNAFeature::space),
            profile.get(SoundDNAFeature::harmonicComplexity),
            profile.get(SoundDNAFeature::bassWeight),
            profile.get(SoundDNAFeature::stereoWidth)};
}

} // namespace pas1::dsp
