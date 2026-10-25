#pragma once

#include <volk.h>

namespace Northrend {

class VulkanDevice;


class VulkanDepthImage final {
public:
    VulkanDepthImage(
        VulkanDevice& device,
        VkExtent2D extent,
        VkFormat format);
    ~VulkanDepthImage();

    VulkanDepthImage(const VulkanDepthImage&) = delete;
    VulkanDepthImage& operator=(const VulkanDepthImage&) = delete;

    VkImageView view() const noexcept;

private:
    void destroy() noexcept;

    VulkanDevice& device;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
};

}
