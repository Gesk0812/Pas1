#pragma once

namespace pas1::presets {

inline constexpr float factoryMasterFloor = 0.75f;

constexpr float compensateFactoryMaster(float master) noexcept {
    return master < factoryMasterFloor ? factoryMasterFloor : master;
}

} // namespace pas1::presets
