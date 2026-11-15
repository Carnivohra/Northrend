#pragma once

#include <volk.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;

class VulkanTransferPage final {
public:
    VulkanTransferPage(VulkanDevice& device, VkDeviceSize capacity);
    ~VulkanTransferPage();

    VulkanTransferPage(const VulkanTransferPage&) = delete;
    VulkanTransferPage& operator=(const VulkanTransferPage&) = delete;

    [[nodiscard]] std::optional<VkDeviceSize> allocate(VkDeviceSize size);
    void release(VkDeviceSize offset, VkDeviceSize size) noexcept;
    void write(std::span<const std::byte> data, VkDeviceSize offset);
    [[nodiscard]] VkBuffer handle() const noexcept;

private:
    void merge_free_ranges() noexcept;

    std::unique_ptr<VulkanBuffer> buffer;
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> freeRanges;
};

}
