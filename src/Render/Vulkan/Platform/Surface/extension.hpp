#pragma once

#include "Backend/Window/handle.hpp"

namespace Northrend::VulkanPlatform {

[[nodiscard]] const char* surface_extension(
    const NativeWindowHandle& handle);

}
