#pragma once

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;

class VulkanMesh final {
public:
    VulkanMesh(VulkanDevice& device, std::span<const std::byte> vertices, std::span<const std::byte> indices,
        std::uint32_t indexCount, VkIndexType indexType);
    ~VulkanMesh();

    VulkanMesh(const VulkanMesh&) = delete;
    VulkanMesh& operator=(const VulkanMesh&) = delete;

    void draw(VkCommandBuffer commandBuffer) const noexcept;

private:
    std::unique_ptr<VulkanBuffer> buffer;
    VkDeviceSize indexOffset;
    std::uint32_t indexCount;
    VkIndexType indexType;
};

}
