#pragma once

#include "Render/Vulkan/buffer.hpp"

#include <cstdint>
#include <memory>

namespace Northrend {

struct VulkanCameraBuffer final {
    std::unique_ptr<VulkanBuffer> buffer;
    void* mapped = nullptr;
    std::uint64_t uploadRevision = 0;
};

}
