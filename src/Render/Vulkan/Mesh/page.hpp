#pragma once

#include <volk.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;

class VulkanMeshPage final {
public:
    VulkanMeshPage(VulkanDevice& device, VkDeviceSize capacity);
    ~VulkanMeshPage();

    VulkanMeshPage(const VulkanMeshPage&) = delete;
    VulkanMeshPage& operator=(const VulkanMeshPage&) = delete;

    [[nodiscard]] std::optional<VkDeviceSize> allocate(VkDeviceSize size);
    void release(VkDeviceSize offset, VkDeviceSize size) noexcept;

    [[nodiscard]] VkBuffer handle() const noexcept;

private:
    void merge_free_ranges() noexcept;

    std::unique_ptr<VulkanBuffer> buffer;
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> freeRanges;
};

}
