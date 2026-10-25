#include "Render/Vulkan/triangle.hpp"

#include "Render/Vulkan/mesh_store.hpp"

#include <array>
#include <cstdint>
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

MeshHandle create(VulkanMeshStore& meshes) {
    return meshes.create(
        std::as_bytes(std::span(Vertices)),
        std::as_bytes(std::span(Indices)),
        static_cast<std::uint32_t>(Indices.size()),
        VK_INDEX_TYPE_UINT16);
}

}
