#include "SoundGenerator.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <iterator>
#include <limits>
#include <string_view>

namespace pas1::generation {
namespace {
float unit(float value) noexcept {
    return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 1.0f);
}

std::string lowerCase(std::string text) {
    std::string result;
    result.reserve(text.size());

    for (std::size_t index = 0; index < text.size();) {
        const auto first = static_cast<unsigned char>(text[index]);
        if (first < 0x80) {
            result.push_back(static_cast<char>(std::tolower(first)));
            ++index;
            continue;
        }

        if (first == 0xd0 && index + 1 < text.size()) {
            const auto second = static_cast<unsigned char>(text[index + 1]);
            if (second == 0x81) { // Ё -> ё
                result.push_back(static_cast<char>(0xd1));
                result.push_back(static_cast<char>(0x91));
                index += 2;
                continue;
            }
            if (second >= 0x90 && second <= 0xaf) { // А-Я -> а-я
                if (second <= 0x9f) {
                    result.push_back(static_cast<char>(0xd0));
                    result.push_back(static_cast<char>(second + 0x20));
                } else {
                    result.push_back(static_cast<char>(0xd1));
                    result.push_back(static_cast<char>(second - 0x20));
                }
                index += 2;
                continue;
            }
        }

        result.push_back(static_cast<char>(first));
        ++index;
    }
    return result;
}

bool contains(const std::string& text, std::string_view word) {
    return text.find(word) != std::string::npos;
}

struct DescriptionIntent {
    bool warm = false;
    bool bright = false;
    bool dark = false;
    bool moving = false;
    bool metallic = false;
    bool aggressive = false;
    bool distortion = false;
    bool wide = false;
    bool spacious = false;
    bool longTail = false;
    bool bass = false;
    bool pad = false;
    bool pluck = false;
    bool musicBox = false;
    bool aulos = false;
    bool lyre = false;
    bool foundMetal = false;
    bool gong = false;
    bool sharpAttack = false;
    bool shortRelease = false;
};

DescriptionIntent analyzeIntent(const std::string& text) {
    DescriptionIntent intent;
    intent.warm = contains(text, "warm") || contains(text, "analog")
                  || contains(text, "soft")
                  || contains(text, "аналог") || contains(text, "мягк")
                  || contains(text, "тёпл")
                  || contains(text, "тепл");
    intent.bright = contains(text, "bright") || contains(text, "glass")
                    || contains(text, "стекл")
                    || contains(text, "светл") || contains(text, "ярк");
    intent.dark = contains(text, "dark") || contains(text, "тёмн")
                  || contains(text, "темн") || contains(text, "мрачн");
    intent.moving = contains(text, "evolving") || contains(text, "moving")
                    || contains(text, "движ") || contains(text, "развива")
                    || contains(text, "эволюц");
    intent.metallic = contains(text, "metallic") || contains(text, "metal")
                      || contains(text, "металл") || contains(text, "металлич");
    intent.aggressive = contains(text, "aggressive") || contains(text, "агрессив")
                        || contains(text, "злой");
    intent.distortion = contains(text, "moderate distortion") || contains(text, "умерен")
                        || contains(text, "distortion") || contains(text, "искаж")
                        || contains(text, "перегруз");
    intent.wide = contains(text, "wide") || contains(text, "stereo")
                  || contains(text, "широк") || contains(text, "стерео");
    intent.longTail = contains(text, "long tail") || contains(text, "длинн")
                      || contains(text, "хвост");
    intent.spacious = contains(text, "spacious") || contains(text, "tail")
                      || contains(text, "space") || contains(text, "простран")
                      || intent.longTail;
    intent.bass = contains(text, "bass") || contains(text, "бас");
    intent.pad = contains(text, "pad") || contains(text, "пэд");
    intent.pluck = contains(text, "pluck") || contains(text, "plucked")
                   || contains(text, "плик") || contains(text, "щипк");
    intent.musicBox = contains(text, "music box") || contains(text, "musicbox")
                      || contains(text, "шкатул");
    intent.aulos = contains(text, "aulos") || contains(text, "reed pipe")
                   || contains(text, "аулос");
    intent.lyre = contains(text, "lyre") || contains(text, "lyra")
                  || contains(text, "лир");
    intent.foundMetal = contains(text, "found metal")
                        || contains(text, "scrap metal")
                        || contains(text, "металлолом")
                        || contains(text, "обломки металла");
    intent.gong = contains(text, "gong") || contains(text, "bronze")
                  || contains(text, "гонг") || contains(text, "бронз")
                  || contains(text, "odyssey") || contains(text, "одиссея")
                  || contains(text, "ancient") || contains(text, "древн");
    intent.sharpAttack = contains(text, "sharp attack")
                         || contains(text, "fast attack")
                         || contains(text, "snappy")
                         || contains(text, "резкая атака")
                         || contains(text, "быстрая атака")
                         || contains(text, "металлическая атака")
                         || (contains(text, "металлич") && contains(text, "атак"))
                         || (contains(text, "резк") && contains(text, "атак"))
                         || (contains(text, "быстр") && contains(text, "атак"))
                         || contains(text, "metallic attack");
    intent.shortRelease = contains(text, "short release")
                          || contains(text, "staccato")
                          || contains(text, "короткий релиз")
                          || contains(text, "короткое затух")
                          || contains(text, "стаккато");
    return intent;
}

bool isDescriptionStopWord(std::string_view word) {
    static constexpr std::string_view stopWords[]{
        "a", "an", "and", "are", "as", "be", "by", "create", "for", "from",
        "i", "in", "it", "make", "me", "my", "of", "on", "or", "please",
        "some", "sound", "synth", "the", "this", "to", "use", "want", "with",
        "с", "со", "и", "а", "но", "в", "во", "на", "из", "для", "от", "до",
        "по", "под", "над", "при", "как", "чтобы", "мне", "мой", "моя", "мои",
        "это", "этот", "эта", "звук", "звучание", "сделай", "создай", "нужен",
        "нужна", "нужно", "надо", "хочу", "добавь", "сделать", "пожалуйста",
        "просто", "вариант", "синтезатор", "пресет"
    };
    return std::find(std::begin(stopWords), std::end(stopWords), word)
           != std::end(stopWords);
}

bool isSupportedDescriptionWord(std::string_view word, const DescriptionIntent& intent) {
    static constexpr std::string_view supportedRoots[]{
        "warm", "analog", "аналог", "soft", "мягк", "тёпл", "тепл",
        "bright", "glass", "светл", "ярк", "стекл", "dark", "тёмн", "темн",
        "мрачн", "evolving", "moving", "движ", "развива", "эволюц",
        "metallic", "metal", "металл", "металлич", "aggressive", "агрессив", "злой",
        "distortion", "moderate", "умерен", "искаж", "перегруз", "wide",
        "width", "широк",
        "spacious", "space", "tail", "хвост", "длинн", "простран",
        "stereo", "стерео",
        "bass", "бас", "pad", "пэд", "pluck", "plucked", "плик", "щипк",
        "musicbox", "шкатул", "aulos", "аулос", "lyre", "lyra", "лир",
        "металлолом", "обломки",
        "gong", "bronze", "гонг", "бронз", "odyssey", "одиссея", "ancient",
        "древн", "sharp", "fast", "snappy", "резк", "быстр", "staccato",
        "затух", "стаккато"
    };
    const auto matchesRoot = [word](std::string_view root) {
        return word.find(root) != std::string_view::npos;
    };
    if (std::any_of(std::begin(supportedRoots), std::end(supportedRoots), matchesRoot))
        return true;
    if (intent.musicBox && (word == "music" || word == "box"))
        return true;
    if (intent.aulos && (word == "reed" || word == "pipe"))
        return true;
    if (intent.foundMetal && (word == "found" || word == "scrap"))
        return true;
    if (intent.sharpAttack
        && (word == "attack" || word.find("атак") != std::string_view::npos))
        return true;
    if (intent.shortRelease
        && (word == "short" || word == "release" || word == "релиз"
            || word.find("коротк") != std::string_view::npos))
        return true;
    return intent.longTail && word == "long";
}

std::vector<std::string> extractUnsupportedWords(const std::string& text) {
    const auto lower = lowerCase(text);
    const auto intent = analyzeIntent(lower);
    std::vector<std::string> unsupported;
    for (std::size_t index = 0; index < lower.size();) {
        const auto byte = static_cast<unsigned char>(lower[index]);
        const auto isAsciiWord = byte < 0x80 && std::isalnum(byte) != 0;
        const auto isCyrillicLead = byte == 0xd0 || byte == 0xd1;
        if (isCyrillicLead
            && (index + 1 >= lower.size()
                || (static_cast<unsigned char>(lower[index + 1]) & 0xc0) != 0x80)) {
            ++index;
            continue;
        }
        if (!isAsciiWord && !isCyrillicLead) {
            ++index;
            continue;
        }

        const auto start = index;
        while (index < lower.size()) {
            const auto current = static_cast<unsigned char>(lower[index]);
            if (current < 0x80) {
                if (std::isalnum(current) == 0)
                    break;
                ++index;
            } else if ((current == 0xd0 || current == 0xd1)
                       && index + 1 < lower.size()) {
                index += 2;
            } else {
                break;
            }
        }
        const auto word = std::string_view{lower}.substr(start, index - start);
        if (word.size() > 2 && !isDescriptionStopWord(word)
            && !isSupportedDescriptionWord(word, intent)) {
            const std::string token{word};
            if (std::find(unsupported.begin(), unsupported.end(), token)
                == unsupported.end())
                unsupported.push_back(token);
        }
    }
    return unsupported;
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
           || id == "ancientinstrument" || id == "ancientmix"
           || id == "ancientbrightness" || id == "ancientdecay"
           || id == "ancientair" || id == "ancientdetune"
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
           || id == "ancientinstrument" || id == "ancientmix"
           || id == "ancientbrightness" || id == "ancientdecay"
           || id == "ancientair" || id == "ancientdetune"
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
    if (contains(genre, "bass") || contains(genre, "бас")) {
        if (id == "filtercutoff")
            return 0.34f + randomUnit(randomState) * 0.16f;
        if (id == "sublevel")
            return 0.52f + randomUnit(randomState) * 0.28f;
        if (id == "attack")
            return 0.02f + randomUnit(randomState) * 0.07f;
    } else if (contains(genre, "pad") || contains(genre, "пэд")) {
        if (id == "attack")
            return 0.18f + randomUnit(randomState) * 0.32f;
        if (id == "release")
            return 0.28f + randomUnit(randomState) * 0.32f;
        if (id == "reverbmix" || id == "chorusmix" || id == "stereowidth")
            return 0.28f + randomUnit(randomState) * 0.3f;
    } else if (contains(genre, "lead") || contains(genre, "лид")) {
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
                   || id == "sublevel" || id == "noiselevel"
                   || startsWith(id, "musicbox") || startsWith(id, "ancient");
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

RecipeInterpretation SoundGenerator::analyzeDescription(const std::string& description) {
    const auto intent = analyzeIntent(lowerCase(description));
    RecipeInterpretation result;
    const auto addTrait = [&result](bool enabled, const char* name) {
        if (enabled)
            result.recognizedTraits.emplace_back(name);
    };
    addTrait(intent.warm, "тёплый / аналоговый");
    addTrait(intent.bright, "яркий / стеклянный");
    addTrait(intent.dark, "тёмный");
    addTrait(intent.moving, "подвижный");
    addTrait(intent.metallic, "металлический тембр");
    addTrait(intent.aggressive, "агрессивный");
    addTrait(intent.distortion, "искажение");
    addTrait(intent.wide, "широкое стерео");
    addTrait(intent.spacious, intent.longTail ? "пространство / длинный хвост"
                                              : "пространственный звук");
    addTrait(intent.bass, "бас");
    addTrait(intent.pad, "пэд");
    addTrait(intent.pluck, "щипковая атака");
    addTrait(intent.musicBox, "музыкальная шкатулка");
    addTrait(intent.aulos, "аулос");
    addTrait(intent.lyre, "лира");
    addTrait(intent.foundMetal, "ударный тембр из найденного металла");
    addTrait(intent.gong, "гонг / древний тембр");
    addTrait(intent.sharpAttack, "короткая резкая атака");
    addTrait(intent.shortRelease, "короткое затухание");
    result.unsupportedWords = extractUnsupportedWords(description);
    return result;
}

std::vector<ParameterValue> SoundGenerator::createRecipe(
    const std::vector<ParameterValue>& base, const std::string& description,
    const RecipeControls& controls, const std::vector<std::string>& lockedParameters) {
    auto result = base;
    const auto text = lowerCase(description);
    const auto intent = analyzeIntent(text);
    auto character = unit(controls.character);
    auto brightness = unit(controls.brightness);
    auto density = unit(controls.density);
    auto movement = unit(controls.movement);
    auto aggression = unit(controls.aggression);
    auto space = unit(controls.space);
    auto complexity = unit(controls.harmonicComplexity);
    auto bassWeight = unit(controls.bassWeight);
    auto stereoWidth = unit(controls.stereoWidth);
    int ancientMode = 0;

    if (intent.warm) {
        character = std::max(character, 0.72f);
        brightness = std::min(brightness, 0.48f);
    }
    if (intent.bright) {
        brightness = std::max(brightness, 0.78f);
        complexity = std::max(complexity, 0.58f);
    }
    if (intent.dark)
        brightness = std::min(brightness, 0.28f);
    if (intent.moving)
        movement = std::max(movement, 0.72f);
    if (intent.metallic)
        complexity = std::max(complexity, 0.8f);
    if (intent.aggressive)
        aggression = std::max(aggression, 0.78f);
    if (intent.distortion)
        aggression = std::max(aggression, 0.48f);
    if (intent.wide)
        stereoWidth = std::max(stereoWidth, 0.78f);
    if (intent.spacious)
        space = std::max(space, intent.longTail ? 0.88f : 0.75f);
    if (intent.bass)
        bassWeight = std::max(bassWeight, 0.8f);
    if (intent.pad) {
        density = std::max(density, 0.62f);
        movement = std::max(movement, 0.42f);
    }
    if (intent.pluck) {
        setRaw(result, "decay", 0.32f);
        setRaw(result, "sustain", 0.2f);
    }
    if (intent.metallic)
        setRaw(result, "fmamount", 0.24f + complexity * 0.48f);
    if (intent.musicBox) {
        setRaw(result, "musicboxmix", 0.9f);
        setRaw(result, "musicboxbrightness", 0.78f);
        setRaw(result, "musicboxdecay", 1.8f);
        setRaw(result, "musicboxresonance", 0.76f);
        setRaw(result, "musicboxinharmonicity", 0.82f);
        setRaw(result, "musicboxstrike", 0.62f);
    }
    if (intent.aulos)
        ancientMode = 1;
    else if (intent.lyre)
        ancientMode = 2;
    else if (intent.foundMetal)
        ancientMode = 4;
    else if (intent.gong)
        ancientMode = 3;

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
    if (intent.pad) {
        setRaw(result, "attack", 0.45f);
        setRaw(result, "release", 1.5f);
    }
    if (intent.pluck || intent.sharpAttack)
        setRaw(result, "attack", 0.003f);
    if (intent.longTail && !intent.shortRelease)
        setRaw(result, "release", std::max(1.8f, blend(0.12f, 2.2f, space)));
    if (intent.shortRelease)
        setRaw(result, "release", 0.12f);
    if (ancientMode > 0) {
        setRaw(result, "ancientinstrument", static_cast<float>(ancientMode));
        setRaw(result, "ancientmix", 0.88f);
        setRaw(result, "ancientbrightness", blend(0.42f, 0.88f, brightness));
        setRaw(result, "ancientdecay", blend(2.2f, 12.0f, space));
        setRaw(result, "ancientair", blend(0.2f, 0.78f, complexity));
        setRaw(result, "ancientdetune", intent.aulos ? 0.5f : 0.24f);
        setRaw(result, "osc1level", 0.12f);
        setRaw(result, "osc2level", 0.035f);
        setRaw(result, "osc3level", 0.02f);
        setRaw(result, "sublevel", 0.0f);
        setRaw(result, "noiselevel", 0.0f);
    }

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
