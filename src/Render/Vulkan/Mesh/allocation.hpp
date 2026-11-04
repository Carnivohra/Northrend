#pragma once

#include <volk.h>

#include <cstddef>

namespace Northrend {

struct VulkanMeshAllocation final {
    std::size_t page;
    VkDeviceSize offset;
    VkDeviceSize size;
};

}
