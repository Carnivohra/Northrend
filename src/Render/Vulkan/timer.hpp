#pragma once

#include "Debug/timing.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Northrend {

class VulkanDevice;

class VulkanGpuTimer final {
public:
    explicit VulkanGpuTimer(VulkanDevice& device);
    ~VulkanGpuTimer();

    VulkanGpuTimer(const VulkanGpuTimer&) = delete;
    VulkanGpuTimer& operator=(const VulkanGpuTimer&) = delete;

    void allocate(std::size_t count);
    void release() noexcept;

    void begin(VkCommandBuffer commandBuffer, std::size_t imageIndex) const noexcept;
    void end(VkCommandBuffer commandBuffer, std::size_t imageIndex) const noexcept;
    void submitted(std::size_t imageIndex) noexcept;

    [[nodiscard]] std::optional<Timing::Summary> collect(std::size_t imageIndex);

private:
    [[nodiscard]] std::uint32_t first_query(std::size_t imageIndex) const noexcept;

    VulkanDevice& device;
    VkQueryPool pool = VK_NULL_HANDLE;
    std::vector<std::uint8_t> pending;
    Timing timing;
    double timestampPeriod = 0.0;
    std::uint64_t timestampMask = 0;
};

}
