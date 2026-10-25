#pragma once

#include <volk.h>

#include <cstddef>
#include <vector>

namespace Northrend {

struct RenderFrame;
class VulkanDevice;
class VulkanMeshStore;
class VulkanMeshPipeline;
class VulkanRenderTarget;

class VulkanCommandContext final {
public:
    explicit VulkanCommandContext(VulkanDevice& device);
    ~VulkanCommandContext();

    VulkanCommandContext(const VulkanCommandContext&) = delete;
    VulkanCommandContext& operator=(const VulkanCommandContext&) = delete;

    void allocate(std::size_t count);
    void release() noexcept;

    void record(
        std::size_t imageIndex,
        const RenderFrame& frame,
        const VulkanRenderTarget& target,
        VulkanMeshPipeline& pipeline,
        const VulkanMeshStore& meshes,
        const VkClearColorValue& clearColor);

    [[nodiscard]] VkCommandBuffer handle(
        std::size_t imageIndex) const noexcept;

private:
    VulkanDevice& device;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> buffers;
};

}
