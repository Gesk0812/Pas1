#include "ModulationMatrix.h"

#include <algorithm>
#include <cmath>

namespace pas1::dsp {

ModulationMatrix::DestinationValues ModulationMatrix::evaluate(
    const SourceValues& sources, const Routes& routes) noexcept {
    DestinationValues outputs{};
    for (const auto& route : routes) {
        if (!route.enabled)
            continue;
        const auto sourceIndex = static_cast<std::size_t>(route.source);
        const auto destinationIndex = static_cast<std::size_t>(route.destination);
        if (sourceIndex >= sourceCount || destinationIndex >= destinationCount)
            continue;
        const auto rawSource = std::isfinite(sources[sourceIndex])
                                   ? std::clamp(sources[sourceIndex], -1.0f, 1.0f)
                                   : 0.0f;
        const auto source = convertPolarity(rawSource, route.polarity);
        const auto amount = std::isfinite(route.amount)
                                ? std::clamp(route.amount, -1.0f, 1.0f)
                                : 0.0f;
        outputs[destinationIndex] = std::clamp(
            outputs[destinationIndex] + source * amount, -1.0f, 1.0f);
    }
    return outputs;
}

float ModulationMatrix::convertPolarity(float bipolarValue,
                                        ModulationPolarity polarity) noexcept {
    const auto source = std::isfinite(bipolarValue)
                            ? std::clamp(bipolarValue, -1.0f, 1.0f)
                            : 0.0f;
    return polarity == ModulationPolarity::unipolar ? (source + 1.0f) * 0.5f : source;
}

void ModulationAccumulator::reset() noexcept {
    sums_.fill(0.0);
    sampleCount_ = 0;
}

void ModulationAccumulator::add(
    const ModulationMatrix::DestinationValues& values) noexcept {
    for (std::size_t index = 0; index < values.size(); ++index)
        if (std::isfinite(values[index]))
            sums_[index] += std::clamp(values[index], -1.0f, 1.0f);
    ++sampleCount_;
}

ModulationMatrix::DestinationValues ModulationAccumulator::average() const noexcept {
    ModulationMatrix::DestinationValues result{};
    if (sampleCount_ == 0)
        return result;
    const auto divisor = static_cast<double>(sampleCount_);
    for (std::size_t index = 0; index < result.size(); ++index)
        result[index] = std::clamp(static_cast<float>(sums_[index] / divisor), -1.0f, 1.0f);
    return result;
}

} // namespace pas1::dsp
