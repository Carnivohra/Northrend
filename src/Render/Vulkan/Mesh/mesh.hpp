#pragma once

#include "Render/Mesh/data.hpp"
#include "Render/Vulkan/Draw/draw.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>

namespace Northrend {

class VulkanMeshArena;

class VulkanMesh final {
public:
    VulkanMesh(VulkanMeshArena& arena, const MeshData& data);
    ~VulkanMesh();

    VulkanMesh(const VulkanMesh&) = delete;
    VulkanMesh& operator=(const VulkanMesh&) = delete;

    [[nodiscard]] VulkanDraw draw(std::uint32_t firstInstance) const;
    [[nodiscard]] VkIndexType index_type() const noexcept;

private:
    [[nodiscard]] static VkIndexType vulkan_index_type(MeshIndexType type) noexcept;

    VulkanMeshArena& arena;
    VkDeviceSize offset;
    VkDeviceSize byteSize;
    VkDeviceSize indexOffset;
    std::uint32_t indexCount;
    VkIndexType indexType;
};

}
