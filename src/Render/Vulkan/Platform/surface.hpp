#pragma once

#include "Backend/Window/handle.hpp"

#include <volk.h>

namespace Northrend::VulkanPlatform {

const char* surface_extension(const NativeWindowHandle& handle);
VkSurfaceKHR create_surface(VkInstance instance, const NativeWindowHandle& handle);

}
