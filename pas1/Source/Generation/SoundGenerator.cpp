#include "SoundGenerator.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <string_view>

namespace pas1::generation {
namespace {
float unit(float value) noexcept {
    return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 1.0f);
}

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool contains(const std::string& text, std::string_view word) {
    return text.find(word) != std::string::npos;
}

bool startsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

float rawToNormalised(const ParameterValue& parameter, float raw) noexcept {
    const auto span = parameter.maximum - parameter.minimum;
    if (!std::isfinite(raw) || !std::isfinite(span) || span <= 0.0f)
        return parameter.defaultNormalizedValue;

    auto proportion = std::clamp((raw - parameter.minimum) / span, 0.0f, 1.0f);
    const auto skew = std::isfinite(parameter.skew) && parameter.skew > 0.0f
                          ? parameter.skew
                          : 1.0f;
    proportion = std::pow(proportion, 1.0f / skew);
    if (parameter.discrete && parameter.stepCount > 1)
        proportion = std::round(proportion * static_cast<float>(parameter.stepCount - 1))
                     / static_cast<float>(parameter.stepCount - 1);
    return unit(proportion);
}

void setRaw(std::vector<ParameterValue>& parameters, std::string_view id, float raw) {
    const auto found = std::find_if(parameters.begin(), parameters.end(),
                                    [id](const auto& parameter) { return parameter.id == id; });
    if (found != parameters.end())
        found->normalizedValue = rawToNormalised(*found, raw);
}

float blend(float low, float high, float amount) noexcept {
    return low + (high - low) * unit(amount);
}

std::uint32_t nextRandom(std::uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

float randomUnit(std::uint32_t& state) noexcept {
    return static_cast<float>(nextRandom(state) >> 8) / 16777215.0f;
}

bool isVariationTarget(std::string_view id) {
    return id == "waveform" || id == "osc1level" || id == "osc1octave"
           || id == "osc1fine" || id == "wavetablescan" || id == "unisonvoices"
           || id == "unisondetune" || id == "osc2wave" || id == "osc2level"
           || id == "osc2detune" || id == "fmamount" || id == "ringamount"
           || id == "musicboxmix" || id == "musicboxbrightness"
           || id == "musicboxdecay" || id == "musicboxresonance"
           || id == "musicboxinharmonicity"
           || id == "musicboxstrike"
           || id == "osc3wave" || id == "osc3level" || id == "pulsewidth"
           || id == "sublevel" || id == "noiselevel" || id == "filtertype"
           || id == "filtercutoff" || id == "filterresonance" || id == "filterdrive"
           || id == "filterenvamount" || id == "filterattack" || id == "filterdecay"
           || id == "filtersustain" || id == "filterrelease" || id == "attack"
           || id == "decay" || id == "sustain" || id == "release"
           || id == "stereowidth" || startsWith(id, "lfo")
           || startsWith(id, "chorus") || startsWith(id, "phaser")
           || startsWith(id, "delay") || startsWith(id, "reverb") || startsWith(id, "eq");
}

bool isTextureTarget(std::string_view id) {
    return id == "wavetablescan" || id == "noiselevel" || id == "osc2wave"
           || id == "fmamount" || id == "ringamount" || id == "filtertype"
           || id == "musicboxmix" || id == "musicboxbrightness"
           || id == "musicboxdecay" || id == "musicboxresonance"
           || id == "musicboxinharmonicity"
           || id == "musicboxstrike"
           || id == "filterresonance" || id == "filterdrive" || id == "pulsewidth"
           || startsWith(id, "chorus") || startsWith(id, "phaser")
           || startsWith(id, "delay") || startsWith(id, "reverb") || startsWith(id, "eq");
}

bool isTemporalTarget(std::string_view id) {
    return id == "attack" || id == "decay" || id == "sustain" || id == "release"
           || startsWith(id, "lfo") || startsWith(id, "chorus")
           || startsWith(id, "delay") || startsWith(id, "reverb");
}

float categoryCenter(std::string_view id, const std::string& genre, std::uint32_t& randomState) {
    if (contains(genre, "bass")) {
        if (id == "filtercutoff")
            return 0.34f + randomUnit(randomState) * 0.16f;
        if (id == "sublevel")
            return 0.52f + randomUnit(randomState) * 0.28f;
        if (id == "attack")
            return 0.02f + randomUnit(randomState) * 0.07f;
    } else if (contains(genre, "pad")) {
        if (id == "attack")
            return 0.18f + randomUnit(randomState) * 0.32f;
        if (id == "release")
            return 0.28f + randomUnit(randomState) * 0.32f;
        if (id == "reverbmix" || id == "chorusmix" || id == "stereowidth")
            return 0.28f + randomUnit(randomState) * 0.3f;
    } else if (contains(genre, "lead")) {
        if (id == "filtercutoff")
            return 0.58f + randomUnit(randomState) * 0.28f;
        if (id == "attack")
            return randomUnit(randomState) * 0.05f;
    }
    return randomUnit(randomState);
}
} // namespace

bool SoundGenerator::belongsToSection(std::string_view id,
                                      VariationSection section) noexcept {
    switch (section) {
        case VariationSection::oscillators:
            return id == "waveform" || startsWith(id, "osc") || id == "wavetablescan"
                   || id == "unisonvoices" || id == "unisondetune" || id == "pulsewidth"
                   || id == "sublevel" || id == "noiselevel" || startsWith(id, "musicbox");
        case VariationSection::filterAndEnvelope:
            return startsWith(id, "filter") || id == "attack" || id == "decay"
                   || id == "sustain" || id == "release";
        case VariationSection::modulation:
            return startsWith(id, "lfo") || startsWith(id, "modroute")
                   || id == "morphx" || id == "morphy";
        case VariationSection::effects:
            return startsWith(id, "distortion") || startsWith(id, "chorus")
                   || startsWith(id, "phaser") || startsWith(id, "delay")
                   || startsWith(id, "reverb") || startsWith(id, "eq")
                   || id == "stereowidth";
    }
    return false;
}

std::vector<ParameterValue> SoundGenerator::createRecipe(
    const std::vector<ParameterValue>& base, const std::string& description,
    const RecipeControls& controls, const std::vector<std::string>& lockedParameters) {
    auto result = base;
    const auto text = lowerCase(description);
    auto character = unit(controls.character);
    auto brightness = unit(controls.brightness);
    auto density = unit(controls.density);
    auto movement = unit(controls.movement);
    auto aggression = unit(controls.aggression);
    auto space = unit(controls.space);
    auto complexity = unit(controls.harmonicComplexity);
    auto bassWeight = unit(controls.bassWeight);
    auto stereoWidth = unit(controls.stereoWidth);

    if (contains(text, "warm") || contains(text, "analog") || contains(text, "мягк")) {
        character = std::max(character, 0.72f);
        brightness = std::min(brightness, 0.48f);
    }
    if (contains(text, "bright") || contains(text, "glass") || contains(text, "светл")) {
        brightness = std::max(brightness, 0.78f);
        complexity = std::max(complexity, 0.58f);
    }
    if (contains(text, "dark") || contains(text, "тёмн") || contains(text, "темн"))
        brightness = std::min(brightness, 0.28f);
    if (contains(text, "evolving") || contains(text, "moving") || contains(text, "движ"))
        movement = std::max(movement, 0.72f);
    if (contains(text, "metallic") || contains(text, "metal"))
        complexity = std::max(complexity, 0.8f);
    if (contains(text, "aggressive") || contains(text, "агрессив") || contains(text, "злой"))
        aggression = std::max(aggression, 0.78f);
    if (contains(text, "moderate distortion") || contains(text, "умерен") || contains(text, "distortion"))
        aggression = std::max(aggression, 0.48f);
    if (contains(text, "wide") || contains(text, "широк"))
        stereoWidth = std::max(stereoWidth, 0.78f);
    if (contains(text, "spacious") || contains(text, "tail") || contains(text, "space"))
        space = std::max(space, 0.75f);
    if (contains(text, "bass") || contains(text, "бас"))
        bassWeight = std::max(bassWeight, 0.8f);
    if (contains(text, "pad") || contains(text, "пэд")) {
        density = std::max(density, 0.62f);
        movement = std::max(movement, 0.42f);
        setRaw(result, "attack", 0.45f);
        setRaw(result, "release", 1.5f);
    }
    if (contains(text, "pluck") || contains(text, "плик")) {
        setRaw(result, "attack", 0.003f);
        setRaw(result, "decay", 0.32f);
        setRaw(result, "sustain", 0.2f);
    }
    if (contains(text, "metallic") || contains(text, "metal"))
        setRaw(result, "fmamount", 0.24f + complexity * 0.48f);
    if (contains(text, "music box") || contains(text, "musicbox")
        || contains(text, "шкатул")) {
        setRaw(result, "musicboxmix", 0.9f);
        setRaw(result, "musicboxbrightness", 0.78f);
        setRaw(result, "musicboxdecay", 1.8f);
        setRaw(result, "musicboxresonance", 0.76f);
        setRaw(result, "musicboxinharmonicity", 0.82f);
        setRaw(result, "musicboxstrike", 0.62f);
    }

    setRaw(result, "filtercutoff", blend(260.0f, 11000.0f, brightness));
    setRaw(result, "filterresonance", blend(0.12f, 0.62f, aggression));
    setRaw(result, "filterdrive", aggression * 0.55f);
    setRaw(result, "distortionmix", aggression * 0.42f);
    setRaw(result, "distortiondrive", 0.18f + aggression * 0.72f);
    setRaw(result, "unisonvoices", 1.0f + std::round(density * 6.0f));
    setRaw(result, "unisondetune", 4.0f + density * stereoWidth * 44.0f);
    setRaw(result, "osc1level", 0.78f);
    setRaw(result, "osc2level", 0.08f + complexity * 0.28f);
    setRaw(result, "osc3level", character * 0.16f);
    setRaw(result, "sublevel", bassWeight * 0.68f);
    setRaw(result, "noiselevel", complexity > 0.65f ? (complexity - 0.65f) * 0.22f : 0.0f);
    setRaw(result, "fmamount", complexity * aggression * 0.52f);
    setRaw(result, "ringamount", complexity * 0.28f);
    setRaw(result, "wavetablescan", complexity * movement * 0.82f);
    setRaw(result, "lfo1rate", 0.08f + movement * 5.8f);
    setRaw(result, "lfo1amount", movement * 0.48f);
    setRaw(result, "lfo1target", movement > 0.04f ? 2.0f : 0.0f);
    setRaw(result, "lfo1wave", movement > 0.66f ? 1.0f : 0.0f);
    setRaw(result, "chorusenabled", stereoWidth > 0.05f ? 1.0f : 0.0f);
    setRaw(result, "chorusmix", stereoWidth * 0.38f);
    setRaw(result, "chorusdepth", stereoWidth);
    setRaw(result, "stereowidth", stereoWidth * 2.0f);
    setRaw(result, "reverbenabled", space > 0.04f ? 1.0f : 0.0f);
    setRaw(result, "reverbmix", space * 0.42f);
    setRaw(result, "reverbdecay", 0.2f + space * 0.68f);
    setRaw(result, "delayenabled", space > 0.45f ? 1.0f : 0.0f);
    setRaw(result, "delaymix", space * 0.24f);
    setRaw(result, "attack", blend(0.004f, 0.7f, movement) * (1.0f - aggression * 0.8f));
    setRaw(result, "release", blend(0.12f, 2.2f, space));

    for (auto& parameter : result) {
        if (std::find(lockedParameters.begin(), lockedParameters.end(), parameter.id)
            == lockedParameters.end())
            continue;
        const auto original = std::find_if(base.begin(), base.end(), [&parameter](const auto& item) {
            return item.id == parameter.id;
        });
        if (original != base.end())
            parameter.normalizedValue = original->normalizedValue;
    }
    return result;
}

std::vector<ParameterValue> SoundGenerator::createVariation(
    const std::vector<ParameterValue>& base, VariationMode mode, float amount,
    std::uint32_t seed, const std::vector<std::string>& lockedParameters,
    const std::string& genreHint) {
    auto result = base;
    amount = unit(amount);
    if (amount <= 0.0f || base.empty())
        return result;

    auto randomState = seed == 0 ? 0x6d2b79f5u : seed;
    const auto genre = lowerCase(genreHint);
    auto effectiveMode = mode;
    if (effectiveMode == VariationMode::surprise)
        effectiveMode = static_cast<VariationMode>(nextRandom(randomState) % 8u);

    for (auto& parameter : result) {
        if (!isVariationTarget(parameter.id)
            || std::find(lockedParameters.begin(), lockedParameters.end(), parameter.id)
                   != lockedParameters.end())
            continue;

        if (effectiveMode == VariationMode::texture && !isTextureTarget(parameter.id))
            continue;
        if (effectiveMode == VariationMode::morph && isTemporalTarget(parameter.id))
            continue;
        if (effectiveMode == VariationMode::genre && genre.empty())
            continue;
        if (effectiveMode == VariationMode::presetMutation && randomUnit(randomState) > 0.32f)
            continue;

        auto target = randomUnit(randomState);
        auto radius = 0.5f;
        switch (effectiveMode) {
            case VariationMode::subtle: radius = 0.08f; break;
            case VariationMode::aggressive: radius = 0.72f; break;
            case VariationMode::similar: radius = 0.15f; break;
            case VariationMode::morph: radius = 0.42f; break;
            case VariationMode::texture: radius = 0.58f; break;
            case VariationMode::presetMutation: radius = 0.35f; break;
            case VariationMode::genre:
                target = categoryCenter(parameter.id, genre, randomState);
                radius = 0.3f;
                break;
            case VariationMode::fullRandom:
            case VariationMode::surprise:
            case VariationMode::seeded:
                radius = 0.5f;
                break;
        }

        const auto change = (target - parameter.normalizedValue) * amount
                            * (effectiveMode == VariationMode::fullRandom
                                   || effectiveMode == VariationMode::seeded
                                   ? 1.0f
                                   : radius * 2.0f);
        parameter.normalizedValue = unit(parameter.normalizedValue + change);
        if (parameter.discrete && parameter.stepCount > 1) {
            const auto steps = static_cast<float>(parameter.stepCount - 1);
            parameter.normalizedValue = std::round(parameter.normalizedValue * steps) / steps;
        }
    }
    return result;
}

} // namespace pas1::generation
