#include "Render/Vulkan/Mesh/mesh.hpp"

#include "Render/Vulkan/Mesh/arena.hpp"
#include "Render/Mesh/vertex.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMesh::VulkanMesh(VulkanMeshArena& arena, const MeshData& data) :
    arena(arena), allocation(), indexType(vulkan_index_type(data.index_type())) {
    const std::span<const std::byte> vertices = data.vertices();
    const std::span<const MeshLevel> sourceLevels = data.levels();
    const std::size_t indexSize =
        this->indexType == VK_INDEX_TYPE_UINT16 ? sizeof(std::uint16_t) : sizeof(std::uint32_t);

    const std::size_t remainder = vertices.size_bytes() % indexSize;
    const std::size_t padding = remainder == 0 ? 0 : indexSize - remainder;
    VkDeviceSize offset = vertices.size_bytes() + padding;
    this->levels.reserve(sourceLevels.size());

    for (const MeshLevel& level : sourceLevels) {
        this->levels.push_back(Level {.indexOffset = offset, .indexCount = level.indexCount});

        if (offset > std::numeric_limits<VkDeviceSize>::max() - level.indices.size_bytes()) {
            throw std::overflow_error("Vulkan mesh size overflow");
        }

        offset += level.indices.size_bytes();
    }

    this->allocation = this->arena.allocate(offset);

    try {
        this->arena.upload(this->allocation, vertices, this->levels.front().indexOffset, sourceLevels);
    } catch (...) {
        this->arena.release(this->allocation);
        throw;
    }
}

VulkanMesh::~VulkanMesh() {
    this->arena.release(this->allocation);
}

VkIndexType VulkanMesh::vulkan_index_type(const MeshIndexType type) noexcept {
    return type == MeshIndexType::unsigned16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}

VulkanDraw VulkanMesh::draw(const std::uint32_t firstInstance, const std::size_t level) const {
    const Level& selected = this->levels.at(level);
    const VkDeviceSize indexSize =
        this->indexType == VK_INDEX_TYPE_UINT16 ? sizeof(std::uint16_t) : sizeof(std::uint32_t);
    const VkDeviceSize absoluteIndexOffset = this->allocation.offset + selected.indexOffset;
    const VkDeviceSize firstIndex = absoluteIndexOffset / indexSize;

    if (this->allocation.offset % sizeof(MeshVertex) != 0) {
        throw std::runtime_error("Vulkan mesh allocation is not aligned to its vertex layout");
    }

    const VkDeviceSize vertexOffset = this->allocation.offset / sizeof(MeshVertex);

    if (firstIndex > std::numeric_limits<std::uint32_t>::max() ||
        vertexOffset > static_cast<VkDeviceSize>(std::numeric_limits<std::int32_t>::max())) {
        throw std::overflow_error("Vulkan mesh arena offset exceeds indexed drawing limits");
    }

    return VulkanDraw {
        .indexCount = selected.indexCount,
        .firstIndex = static_cast<std::uint32_t>(firstIndex),
        .vertexOffset = static_cast<std::int32_t>(vertexOffset),
        .firstInstance = firstInstance,
        .detailLevel = 0,
        .material = {}
    };
}

VkIndexType VulkanMesh::index_type() const noexcept {
    return this->indexType;
}

std::size_t VulkanMesh::page() const noexcept {
    return this->allocation.page;
}

}
