#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace pas1::generation {

enum class SoundTrait : std::size_t {
    warm,
    bright,
    dark,
    airy,
    metallic,
    richHarmonics,
    sineWave,
    triangleWave,
    sawWave,
    squareWave,
    pulseWave,
    organWave,
    formantWave,
    bellWave,
    noise,
    bass,
    sub,
    pluck,
    pad,
    strings,
    brass,
    choir,
    detuned,
    unison,
    sync,
    fm,
    ringMod,
    glide,
    shortAttack,
    slowAttack,
    shortRelease,
    longRelease,
    wide,
    space,
    reverb,
    delay,
    chorus,
    phaser,
    vibrato,
    tremolo,
    lowpass,
    highpass,
    bandpass,
    resonant,
    distortion,
    musicBox,
    aulos,
    lyre,
    bronzeGong,
    foundMetal,
    count
};

inline constexpr std::size_t soundTraitCount =
    static_cast<std::size_t>(SoundTrait::count);

struct SoundTraitWord {
    std::string_view term;
    SoundTrait trait;
};

const std::vector<SoundTraitWord>& soundTraitDictionary();
std::string_view soundTraitLabel(SoundTrait trait) noexcept;
std::string_view soundTraitDescription(SoundTrait trait) noexcept;

} // namespace pas1::generation
