#pragma once

#include "Backend/Window/Handle/x11.hpp"

#include <volk.h>

namespace Northrend::VulkanPlatform {

VkSurfaceKHR create_x11_surface(VkInstance instance, const X11WindowHandle& handle);

}
