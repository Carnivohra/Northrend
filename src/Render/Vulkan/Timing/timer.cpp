#include "Render/Vulkan/Timing/timer.hpp"

#include "Render/Vulkan/device.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanGpuTimer::VulkanGpuTimer(VulkanDevice& device) : device(device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(this->device.physical(), &deviceProperties);

    std::uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(this->device.physical(), &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> familyProperties(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(this->device.physical(), &familyCount, familyProperties.data());

    const std::uint32_t family = this->device.graphics_family();

    if (family >= familyProperties.size() || familyProperties[family].timestampValidBits == 0) {
        return;
    }

    const std::uint32_t validBits = familyProperties[family].timestampValidBits;
    this->timestampPeriod = deviceProperties.limits.timestampPeriod;
    this->timestampMask = validBits >= 64 ? std::numeric_limits<std::uint64_t>::max() : (1ULL << validBits) - 1ULL;
}

VulkanGpuTimer::~VulkanGpuTimer() {
    this->release();
}

void VulkanGpuTimer::allocate(const std::size_t count) {
    this->release();

    if (this->timestampMask == 0 || count == 0) {
        return;
    }

    if (count > std::numeric_limits<std::uint32_t>::max() / 2) {
        throw std::overflow_error("Vulkan GPU timer exceeds the query index range");
    }

    const VkQueryPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = static_cast<std::uint32_t>(count * 2),
        .pipelineStatistics = 0
    };
    const VkResult result = vkCreateQueryPool(this->device.handle(), &createInfo, nullptr, &this->pool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan GPU timer: " + std::to_string(result));
    }

    this->pending.assign(count, false);
    this->interval = Clock::now();
    this->elapsedMilliseconds = 0.0;
    this->samples = 0;
}

void VulkanGpuTimer::release() noexcept {
    this->pending.clear();
    this->elapsedMilliseconds = 0.0;
    this->samples = 0;

    if (this->pool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(this->device.handle(), this->pool, nullptr);
        this->pool = VK_NULL_HANDLE;
    }
}

void VulkanGpuTimer::begin(const VkCommandBuffer commandBuffer, const std::size_t imageIndex) const noexcept {
    if (this->pool == VK_NULL_HANDLE) {
        return;
    }

    const std::uint32_t firstQuery = this->first_query(imageIndex);
    vkCmdResetQueryPool(commandBuffer, this->pool, firstQuery, 2);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, this->pool, firstQuery);
}

void VulkanGpuTimer::end(const VkCommandBuffer commandBuffer, const std::size_t imageIndex) const noexcept {
    if (this->pool == VK_NULL_HANDLE) {
        return;
    }

    vkCmdWriteTimestamp(
        commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, this->pool, this->first_query(imageIndex) + 1);
}

void VulkanGpuTimer::submitted(const std::size_t imageIndex) noexcept {
    if (this->pool != VK_NULL_HANDLE) {
        this->pending[imageIndex] = true;
    }
}

std::optional<double> VulkanGpuTimer::collect(const std::size_t imageIndex) {
    if (this->pool == VK_NULL_HANDLE || !this->pending[imageIndex]) {
        return std::nullopt;
    }

    std::array<std::uint64_t, 2> timestamps {};
    const VkResult result = vkGetQueryPoolResults(this->device.handle(), this->pool, this->first_query(imageIndex),
        static_cast<std::uint32_t>(timestamps.size()), sizeof(timestamps), timestamps.data(), sizeof(std::uint64_t),
        VK_QUERY_RESULT_64_BIT);

    if (result == VK_NOT_READY) {
        return std::nullopt;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to read the Vulkan GPU timer: " + std::to_string(result));
    }

    this->pending[imageIndex] = false;
    const std::uint64_t ticks = (timestamps[1] - timestamps[0]) & this->timestampMask;
    this->elapsedMilliseconds += static_cast<double>(ticks) * this->timestampPeriod / 1'000'000.0;
    ++this->samples;

    const TimePoint now = Clock::now();

    if (now - this->interval < std::chrono::seconds(1)) {
        return std::nullopt;
    }

    const double average = this->elapsedMilliseconds / static_cast<double>(this->samples);
    this->interval = now;
    this->elapsedMilliseconds = 0.0;
    this->samples = 0;

    return average;
}

std::uint32_t VulkanGpuTimer::first_query(const std::size_t imageIndex) const noexcept {
    return static_cast<std::uint32_t>(imageIndex * 2);
}

}
