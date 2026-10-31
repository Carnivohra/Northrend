#pragma once

#include <volk.h>

namespace Northrend {

class VulkanDevice;

class VulkanTransferContext final {
public:
    explicit VulkanTransferContext(VulkanDevice& device);
    ~VulkanTransferContext();

    VulkanTransferContext(const VulkanTransferContext&) = delete;
    VulkanTransferContext& operator=(const VulkanTransferContext&) = delete;

    void copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size, VkDeviceSize sourceOffset,
        VkDeviceSize destinationOffset);

private:
    void destroy() noexcept;

    VulkanDevice& device;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};

}
