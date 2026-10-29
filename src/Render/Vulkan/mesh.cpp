#include "Render/Vulkan/mesh.hpp"

#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace Northrend {

VulkanMesh::VulkanMesh(VulkanDevice& device, std::span<const std::byte> vertices, std::span<const std::byte> indices,
    std::uint32_t indexCount, VkIndexType indexType) :
    indexOffset(0), indexCount(indexCount), indexType(indexType) {
    if (vertices.empty() || indices.empty() || this->indexCount == 0) {
        throw std::invalid_argument("Vulkan mesh data must not be empty");
    }

    std::size_t indexSize = 0;

    if (this->indexType == VK_INDEX_TYPE_UINT16) {
        indexSize = sizeof(std::uint16_t);
    } else if (this->indexType == VK_INDEX_TYPE_UINT32) {
        indexSize = sizeof(std::uint32_t);
    } else {
        throw std::invalid_argument("Vulkan mesh index type is unsupported");
    }

    if (indices.size_bytes() != this->indexCount * indexSize) {
        throw std::invalid_argument("Vulkan mesh index data has an invalid size");
    }

    const std::size_t remainder = vertices.size_bytes() % indexSize;
    const std::size_t padding = remainder == 0 ? 0 : indexSize - remainder;
    const std::size_t indexOffset = vertices.size_bytes() + padding;
    std::vector<std::byte> data(indexOffset + indices.size_bytes());
    std::memcpy(data.data(), vertices.data(), vertices.size_bytes());
    std::memcpy(data.data() + indexOffset, indices.data(), indices.size_bytes());

    this->buffer =
        VulkanBuffer::upload(device, data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    this->indexOffset = indexOffset;
}

VulkanMesh::~VulkanMesh() = default;

void VulkanMesh::draw(VkCommandBuffer commandBuffer) const noexcept {
    const VkBuffer buffer = this->buffer->handle();
    constexpr VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &vertexOffset);
    vkCmdBindIndexBuffer(commandBuffer, buffer, this->indexOffset, this->indexType);
    vkCmdDrawIndexed(commandBuffer, this->indexCount, 1, 0, 0, 0);
}

}
