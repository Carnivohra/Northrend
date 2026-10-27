#pragma once

#include "Backend/Window/Handle/handle.hpp"

#include <volk.h>

namespace Northrend {

class VulkanInstance;

class VulkanSurface final {
public:
    VulkanSurface(
        VulkanInstance& instance,
        const NativeWindowHandle& windowHandle);
    ~VulkanSurface();

    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

    [[nodiscard]] VkSurfaceKHR handle() const noexcept;

private:
    VulkanInstance& instance;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};

}
