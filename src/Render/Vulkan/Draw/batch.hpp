#pragma once

#include "Render/Vulkan/Draw/draw.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

struct RenderFrame;
class VulkanBuffer;
class VulkanDevice;
class VulkanMeshStore;

class VulkanDrawBatch final {
public:
    explicit VulkanDrawBatch(VulkanDevice& device) noexcept;
    ~VulkanDrawBatch();

    VulkanDrawBatch(const VulkanDrawBatch&) = delete;
    VulkanDrawBatch& operator=(const VulkanDrawBatch&) = delete;

    void allocate(std::size_t count);
    void release() noexcept;
    void update(std::size_t imageIndex, const RenderFrame& frame, const VulkanMeshStore& meshes);
    void draw(VkCommandBuffer commandBuffer, std::size_t imageIndex, const VulkanMeshStore& meshes) const noexcept;

private:
    struct Storage final {
        std::unique_ptr<VulkanBuffer> buffer;
        std::unique_ptr<VulkanBuffer> staging;
        std::vector<VulkanDraw> unsigned16;
        std::vector<VulkanDraw> unsigned32;
        std::uint64_t instanceRevision = 0;
        std::uint64_t meshRevision = 0;
        bool initialized = false;
    };

    static constexpr VkDeviceSize InitialCapacity = 64ULL * 1024ULL;

    [[nodiscard]] static VkDeviceSize capacity(VkDeviceSize current, VkDeviceSize required);
    static void draw_group(VkCommandBuffer commandBuffer, const std::vector<VulkanDraw>& draws) noexcept;

    VulkanDevice& device;
    std::vector<Storage> storage;
};

}
