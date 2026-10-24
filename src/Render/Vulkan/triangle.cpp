#include "Render/Vulkan/triangle.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/mesh.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace Northrend::VulkanTriangle {

namespace {

constexpr std::array Vertices{
    Vertex{
        .position = { -1.0F, -1.0F, 0.0F },
        .color = { 0.20F, 0.55F, 1.00F }
    },
    Vertex{
        .position = { 1.0F, -1.0F, 0.0F },
        .color = { 0.20F, 1.00F, 0.65F }
    },
    Vertex{
        .position = { 0.0F, 1.0F, 0.0F },
        .color = { 0.90F, 0.35F, 1.00F }
    }
};

constexpr std::array<std::uint16_t, 3> Indices{ 0, 1, 2 };

}

std::unique_ptr<VulkanMesh> create(VulkanDevice& device) {
    return std::make_unique<VulkanMesh>(
        device,
        std::as_bytes(std::span(Vertices)),
        std::as_bytes(std::span(Indices)),
        static_cast<std::uint32_t>(Indices.size()),
        VK_INDEX_TYPE_UINT16);
}

}
