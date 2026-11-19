#pragma once

#include "Math/matrix.hpp"
#include "Render/Vulkan/Draw/draw.hpp"

#include <cstdint>
#include <cstddef>

namespace Northrend {

struct VulkanInstanceDraw final {
    VulkanDraw draw;
    Matrix4 transform;
    std::uint32_t blendMode;
    std::size_t visibilityIndex;
    bool doubleSided;
};

}
