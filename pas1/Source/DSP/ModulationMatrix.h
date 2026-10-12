#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pas1::dsp {

enum class ModulationSource : std::uint8_t {
    lfo1,
    lfo2,
    lfo3,
    lfo4,
    ampEnvelope,
    filterEnvelope,
    velocity,
    modWheel,
    pitchBend,
    keyTracking,
    randomPerNote,
    count
};

enum class ModulationDestination : std::uint8_t {
    pitch,
    oscillator1Level,
    wavetablePosition,
    filterCutoff,
    resonance,
    fmAmount,
    pan,
    distortionMix,
    reverbMix,
    morphX,
    morphY,
    count
};

enum class ModulationPolarity : std::uint8_t {
    bipolar,
    unipolar
};

struct ModulationRoute {
    ModulationSource source = ModulationSource::lfo1;
    ModulationDestination destination = ModulationDestination::pitch;
    float amount = 0.0f;
    ModulationPolarity polarity = ModulationPolarity::bipolar;
    bool enabled = false;
};

class ModulationMatrix {
public:
    static constexpr std::size_t routeCount = 16;
    static constexpr std::size_t sourceCount =
        static_cast<std::size_t>(ModulationSource::count);
    static constexpr std::size_t destinationCount =
        static_cast<std::size_t>(ModulationDestination::count);

    using SourceValues = std::array<float, sourceCount>;
    using DestinationValues = std::array<float, destinationCount>;
    using Routes = std::array<ModulationRoute, routeCount>;

    static DestinationValues evaluate(const SourceValues& sources,
                                      const Routes& routes) noexcept;
    static float convertPolarity(float bipolarValue, ModulationPolarity polarity) noexcept;
};

class ModulationAccumulator {
public:
    void reset() noexcept;
    void add(const ModulationMatrix::DestinationValues& values) noexcept;
    ModulationMatrix::DestinationValues average() const noexcept;

private:
    std::array<double, ModulationMatrix::destinationCount> sums_{};
    std::uint64_t sampleCount_ = 0;
};

} // namespace pas1::dsp
