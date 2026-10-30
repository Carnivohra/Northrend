#pragma once

#include "Render/Mesh/data.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;

class VulkanMesh final {
public:
    VulkanMesh(VulkanDevice& device, const MeshData& data);
    ~VulkanMesh();

    VulkanMesh(const VulkanMesh&) = delete;
    VulkanMesh& operator=(const VulkanMesh&) = delete;

    void draw(VkCommandBuffer commandBuffer) const noexcept;

private:
    [[nodiscard]] static VkIndexType index_type(MeshIndexType type) noexcept;

    std::unique_ptr<VulkanBuffer> buffer;
    VkDeviceSize indexOffset;
    std::uint32_t indexCount;
    VkIndexType indexType;
};

}
