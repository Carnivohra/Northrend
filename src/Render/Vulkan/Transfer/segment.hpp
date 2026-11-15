#pragma once

#include <volk.h>

#include <cstddef>
#include <span>

namespace Northrend {

struct VulkanTransferSegment final {
    std::span<const std::byte> data;
    VkDeviceSize offset = 0;
};

}
