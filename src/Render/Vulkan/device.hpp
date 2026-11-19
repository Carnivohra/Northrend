#pragma once

#include "Render/Vulkan/physical.hpp"
#include "Render/Vulkan/Transfer/segment.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Northrend {

class VulkanTransferContext;

class VulkanDevice final {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    [[nodiscard]] VkPhysicalDevice physical() const noexcept;
    [[nodiscard]] VkDevice handle() const noexcept;
    [[nodiscard]] VkQueue graphics_queue() const noexcept;
    [[nodiscard]] VkQueue present_queue() const noexcept;
    [[nodiscard]] VkQueue transfer_queue() const noexcept;
    [[nodiscard]] std::uint32_t graphics_family() const noexcept;
    [[nodiscard]] std::uint32_t present_family() const noexcept;
    [[nodiscard]] std::uint32_t transfer_family() const noexcept;
    [[nodiscard]] bool dedicated_transfer_queue() const noexcept;
    [[nodiscard]] bool multi_draw_indirect() const noexcept;
    [[nodiscard]] bool extended_dynamic_state() const noexcept;
    [[nodiscard]] VkFormat depth_format() const;
    [[nodiscard]] std::uint32_t memory_type(std::uint32_t typeBits, VkMemoryPropertyFlags properties) const;

    void prepare_transfers(std::size_t batchCount);
    void upload_buffer(VkBuffer destination, VkDeviceSize destinationOffset, VkDeviceSize size,
        std::span<const VulkanTransferSegment> segments);
    void upload_texture(VkImage destination, std::span<const VkBufferImageCopy> regions, std::uint32_t mipLevels,
        std::uint32_t layers, VkDeviceSize size, std::span<const VulkanTransferSegment> segments);
    [[nodiscard]] VkCommandBuffer flush_transfers(std::size_t batch, std::uint64_t submissionSerial);
    void collect_transfers(std::uint64_t completedSerial) noexcept;
    [[nodiscard]] bool transfers_pending() const noexcept;
    void wait_idle() const noexcept;

private:
    VulkanPhysicalDevice physicalDevice;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    bool supportsMultiDrawIndirect = false;
    bool supportsExtendedDynamicState = false;
    std::unique_ptr<VulkanTransferContext> transfer;
};

}
