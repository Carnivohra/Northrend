#include "Render/Vulkan/Platform/Surface/extension.hpp"

#include <volk.h>

namespace Northrend::VulkanPlatform {

const char* surface_extension(const NativeWindowHandle&) {
    return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

}
