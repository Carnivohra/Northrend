#include "Render/Vulkan/Mesh/mesh.hpp"

#include "Render/Vulkan/Mesh/arena.hpp"
#include "Math/vector.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMesh::VulkanMesh(VulkanMeshArena& arena, const MeshData& data) :
    arena(arena),
    offset(0),
    byteSize(0),
    indexOffset(0),
    indexCount(data.index_count()),
    indexType(vulkan_index_type(data.index_type())) {
    const std::span<const std::byte> vertices = data.vertices();
    const std::span<const std::byte> indices = data.indices();
    const std::size_t indexSize = this->indexType == VK_INDEX_TYPE_UINT16 ? sizeof(std::uint16_t) :
        sizeof(std::uint32_t);

    const std::size_t remainder = vertices.size_bytes() % indexSize;
    const std::size_t padding = remainder == 0 ? 0 : indexSize - remainder;
    this->indexOffset = vertices.size_bytes() + padding;

    if (this->indexOffset > std::numeric_limits<VkDeviceSize>::max() - indices.size_bytes()) {
        throw std::overflow_error("Vulkan mesh size overflow");
    }

    this->byteSize = this->indexOffset + indices.size_bytes();
    this->offset = this->arena.allocate(this->byteSize);

    try {
        this->arena.upload(this->offset, vertices, this->indexOffset, indices);
    } catch (...) {
        this->arena.release(this->offset, this->byteSize);
        throw;
    }
}

VulkanMesh::~VulkanMesh() {
    this->arena.release(this->offset, this->byteSize);
}

VkIndexType VulkanMesh::vulkan_index_type(const MeshIndexType type) noexcept {
    return type == MeshIndexType::unsigned16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}

VulkanDraw VulkanMesh::draw(const std::uint32_t firstInstance) const {
    const VkDeviceSize indexSize = this->indexType == VK_INDEX_TYPE_UINT16 ? sizeof(std::uint16_t) :
        sizeof(std::uint32_t);
    const VkDeviceSize absoluteIndexOffset = this->offset + this->indexOffset;
    const VkDeviceSize firstIndex = absoluteIndexOffset / indexSize;
    const VkDeviceSize vertexOffset = this->offset / sizeof(Vector3);

    if (firstIndex > std::numeric_limits<std::uint32_t>::max() ||
        vertexOffset > static_cast<VkDeviceSize>(std::numeric_limits<std::int32_t>::max())) {
        throw std::overflow_error("Vulkan mesh arena offset exceeds indexed drawing limits");
    }

    return VulkanDraw {
        .indexCount = this->indexCount,
        .firstIndex = static_cast<std::uint32_t>(firstIndex),
        .vertexOffset = static_cast<std::int32_t>(vertexOffset),
        .firstInstance = firstInstance
    };
}

VkIndexType VulkanMesh::index_type() const noexcept {
    return this->indexType;
}

}
