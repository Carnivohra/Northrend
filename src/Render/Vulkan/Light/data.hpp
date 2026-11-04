#pragma once

#include <array>
#include <type_traits>

namespace Northrend {

struct alignas(16) VulkanLightData final {
    std::array<float, 4> directionAmbient;
    std::array<float, 4> colorIntensity;
};

static_assert(sizeof(VulkanLightData) == sizeof(float) * 8);
static_assert(std::is_trivially_copyable_v<VulkanLightData>);
static_assert(std::is_standard_layout_v<VulkanLightData>);

}
