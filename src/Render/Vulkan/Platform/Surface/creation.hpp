#pragma once

#include "Backend/Window/Handle/handle.hpp"

#include <volk.h>

namespace Northrend::VulkanPlatform {

[[nodiscard]] VkSurfaceKHR create_surface(
    VkInstance instance,
    const NativeWindowHandle& handle);

}
