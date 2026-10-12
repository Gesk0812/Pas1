#pragma once

#include "Generation/SoundGenerator.h"

#include <array>
#include <cstddef>
#include <vector>

namespace pas1::dsp {

enum class SoundDNAFeature : std::size_t {
    brightness,
    harmonicComplexity,
    density,
    movement,
    aggression,
    stereoWidth,
    bassWeight,
    space,
    texture,
    envelopeCharacter,
    count
};

struct SoundDNAProfile {
    static constexpr std::size_t featureCount = static_cast<std::size_t>(SoundDNAFeature::count);
    std::array<float, featureCount> values{};

    float get(SoundDNAFeature feature) const noexcept;
};

class SoundDNA {
public:
    static SoundDNAProfile analyze(
        const std::vector<pas1::generation::ParameterValue>& parameters);
    static float distance(const SoundDNAProfile& first,
                         const SoundDNAProfile& second) noexcept;
    static SoundDNAProfile blend(const SoundDNAProfile& first,
                                 const SoundDNAProfile& second, float amount) noexcept;
    static pas1::generation::RecipeControls toRecipeControls(
        const SoundDNAProfile& profile) noexcept;
};

} // namespace pas1::dsp
