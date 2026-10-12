#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace pas1::dsp {

enum class MorphParameter : std::size_t {
    oscillator1Level,
    oscillator2Level,
    oscillator3Level,
    wavetableScan,
    unisonDetune,
    filterCutoff,
    filterResonance,
    attack,
    release,
    lfoDepth,
    fmAmount,
    filterDrive,
    distortionMix,
    stereoWidth,
    reverbMix,
    count
};

class MorphMatrix {
public:
    static constexpr std::size_t parameterCount = static_cast<std::size_t>(MorphParameter::count);
    using Values = std::array<float, parameterCount>;

    MorphMatrix() noexcept;
    void captureCorner(int corner, const Values& values) noexcept;
    Values getCorner(int corner) const noexcept;
    void setEnabled(bool enabled) noexcept;
    bool isEnabled() const noexcept;
    void setLocked(std::size_t parameter, bool locked) noexcept;
    bool isLocked(std::size_t parameter) const noexcept;
    Values process(float x, float y, const Values& current,
                   double sampleRate, int blockSize) noexcept;
    void randomizeCorners(std::uint32_t seed) noexcept;

private:
    static float interpolate(float a, float b, float c, float d, float x, float y,
                              bool logarithmic) noexcept;
    static std::uint32_t nextRandom(std::uint32_t& state) noexcept;

    std::array<std::array<std::atomic<float>, parameterCount>, 4> corners_{};
    std::array<std::atomic<bool>, parameterCount> locked_{};
    std::atomic<bool> enabled_{false};
    Values smoothedValues_{};
    std::atomic<bool> smoothingInitialized_{false};
};

} // namespace pas1::dsp
