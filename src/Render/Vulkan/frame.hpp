#pragma once

#include <volk.h>

namespace Northrend {

class VulkanFrame final {
public:
    explicit VulkanFrame(VkDevice device);
    ~VulkanFrame();

    VulkanFrame(const VulkanFrame&) = delete;
    VulkanFrame& operator=(const VulkanFrame&) = delete;

    VkSemaphore image_available() const noexcept;
    VkSemaphore render_finished() const noexcept;
    VkFence fence() const noexcept;

private:
    void destroy() noexcept;

    VkDevice device = VK_NULL_HANDLE;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence renderFence = VK_NULL_HANDLE;
};

}
