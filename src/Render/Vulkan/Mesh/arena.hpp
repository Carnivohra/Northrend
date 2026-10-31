#pragma once

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Northrend {

class VulkanBuffer;
class VulkanDevice;

class VulkanMeshArena final {
public:
    explicit VulkanMeshArena(VulkanDevice& device) noexcept;
    ~VulkanMeshArena();

    VulkanMeshArena(const VulkanMeshArena&) = delete;
    VulkanMeshArena& operator=(const VulkanMeshArena&) = delete;

    [[nodiscard]] VkDeviceSize allocate(VkDeviceSize size);
    void release(VkDeviceSize offset, VkDeviceSize size) noexcept;
    void upload(VkDeviceSize offset, std::span<const std::byte> vertices, VkDeviceSize indexOffset,
        std::span<const std::byte> indices);

    [[nodiscard]] VkBuffer handle() const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;

private:
    static constexpr VkDeviceSize Alignment = 12;
    static constexpr VkDeviceSize InitialCapacity = 64ULL * 1024ULL * 1024ULL;
    static constexpr VkDeviceSize InitialStagingCapacity = 1ULL * 1024ULL * 1024ULL;

    [[nodiscard]] static VkDeviceSize aligned(VkDeviceSize size);
    [[nodiscard]] static VkDeviceSize grown_capacity(VkDeviceSize capacity, VkDeviceSize required);
    void grow(VkDeviceSize required);
    void grow_staging(VkDeviceSize required);
    void merge_free_ranges() noexcept;

    VulkanDevice& device;
    std::unique_ptr<VulkanBuffer> buffer;
    std::unique_ptr<VulkanBuffer> staging;
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> freeRanges;
    std::uint64_t currentRevision = 0;
};

}
