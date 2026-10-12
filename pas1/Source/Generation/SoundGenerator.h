#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pas1::generation {

struct ParameterValue {
    std::string id;
    float normalizedValue = 0.0f;
    float defaultNormalizedValue = 0.0f;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float skew = 1.0f;
    int stepCount = 0;
    bool discrete = false;
};

struct RecipeControls {
    float character = 0.5f;
    float brightness = 0.5f;
    float density = 0.35f;
    float movement = 0.35f;
    float aggression = 0.25f;
    float space = 0.25f;
    float harmonicComplexity = 0.4f;
    float bassWeight = 0.4f;
    float stereoWidth = 0.35f;
};

struct RecipeInterpretation {
    std::vector<std::string> recognizedTraits;
    std::vector<std::string> unsupportedWords;
};

enum class VariationMode {
    fullRandom,
    subtle,
    aggressive,
    genre,
    morph,
    texture,
    presetMutation,
    similar,
    surprise,
    seeded
};

enum class VariationSection {
    oscillators,
    filterAndEnvelope,
    modulation,
    effects
};

class SoundGenerator {
public:
    static RecipeInterpretation analyzeDescription(const std::string& description);

    static std::vector<ParameterValue> createRecipe(
        const std::vector<ParameterValue>& base, const std::string& description,
        const RecipeControls& controls,
        const std::vector<std::string>& lockedParameters = {});

    static std::vector<ParameterValue> createVariation(
        const std::vector<ParameterValue>& base, VariationMode mode, float amount,
        std::uint32_t seed, const std::vector<std::string>& lockedParameters,
        const std::string& genreHint = {});

    static bool belongsToSection(std::string_view parameterId,
                                 VariationSection section) noexcept;
};

} // namespace pas1::generation
