#pragma once

#include "Backend/Window/Handle/wayland.hpp"

#include <volk.h>

namespace Northrend::VulkanPlatform {

VkSurfaceKHR create_wayland_surface(VkInstance instance, const WaylandWindowHandle& handle);

}
