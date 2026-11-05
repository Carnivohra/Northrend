#pragma once

#include "Render/Material/material.hpp"

#include <cstdint>

namespace Northrend {

struct VulkanDraw final {
    std::uint32_t indexCount;
    std::uint32_t firstIndex;
    std::int32_t vertexOffset;
    std::uint32_t firstInstance;
    MaterialHandle material;
};

}
