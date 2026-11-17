#pragma once

#include <cstdint>

namespace Northrend {

struct VulkanMaterialConstants final {
    float textureMipBias = 0.0F;
    float alphaMipBias = 0.0F;
    std::uint32_t shader = 0;
    std::uint32_t blendMode = 0;
    std::uint32_t flags = 0;
    std::uint32_t textureCount = 0;
};

}
