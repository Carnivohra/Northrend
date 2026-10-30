#include "Render/Vulkan/Mesh/mesh.hpp"

#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace Northrend {

VulkanMesh::VulkanMesh(VulkanDevice& device, const MeshData& data) :
    indexOffset(0), indexCount(data.index_count()), indexType(index_type(data.index_type())) {
    const std::span<const std::byte> vertices = data.vertices();
    const std::span<const std::byte> indices = data.indices();
    const std::size_t indexSize = this->indexType == VK_INDEX_TYPE_UINT16 ? sizeof(std::uint16_t) :
        sizeof(std::uint32_t);

    const std::size_t remainder = vertices.size_bytes() % indexSize;
    const std::size_t padding = remainder == 0 ? 0 : indexSize - remainder;
    const std::size_t indexOffset = vertices.size_bytes() + padding;
    std::vector<std::byte> bufferData(indexOffset + indices.size_bytes());
    std::memcpy(bufferData.data(), vertices.data(), vertices.size_bytes());
    std::memcpy(bufferData.data() + indexOffset, indices.data(), indices.size_bytes());

    this->buffer =
        VulkanBuffer::upload(device, bufferData, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    this->indexOffset = indexOffset;
}

VulkanMesh::~VulkanMesh() = default;

VkIndexType VulkanMesh::index_type(const MeshIndexType type) noexcept {
    return type == MeshIndexType::unsigned16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}

void VulkanMesh::draw(VkCommandBuffer commandBuffer) const noexcept {
    const VkBuffer buffer = this->buffer->handle();
    constexpr VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &vertexOffset);
    vkCmdBindIndexBuffer(commandBuffer, buffer, this->indexOffset, this->indexType);
    vkCmdDrawIndexed(commandBuffer, this->indexCount, 1, 0, 0, 0);
}

}
