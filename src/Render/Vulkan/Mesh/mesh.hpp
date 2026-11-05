#pragma once

#include "Render/Mesh/data.hpp"
#include "Render/Vulkan/Draw/draw.hpp"
#include "Render/Vulkan/Mesh/allocation.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

class VulkanMeshArena;

class VulkanMesh final {
public:
    VulkanMesh(VulkanMeshArena& arena, const MeshData& data);
    ~VulkanMesh();

    VulkanMesh(const VulkanMesh&) = delete;
    VulkanMesh& operator=(const VulkanMesh&) = delete;

    [[nodiscard]] VulkanDraw draw(std::uint32_t firstInstance, std::size_t level) const;
    [[nodiscard]] VkIndexType index_type() const noexcept;
    [[nodiscard]] std::size_t page() const noexcept;

private:
    struct Level final {
        VkDeviceSize indexOffset;
        std::uint32_t indexCount;
    };

    [[nodiscard]] static VkIndexType vulkan_index_type(MeshIndexType type) noexcept;

    VulkanMeshArena& arena;
    VulkanMeshAllocation allocation;
    std::vector<Level> levels;
    VkIndexType indexType;
};

}
