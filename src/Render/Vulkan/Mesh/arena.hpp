#pragma once

#include "Render/Mesh/level.hpp"
#include "Render/Vulkan/Mesh/allocation.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanMeshPage;

class VulkanMeshArena final {
public:
    explicit VulkanMeshArena(VulkanDevice& device) noexcept;
    ~VulkanMeshArena();

    VulkanMeshArena(const VulkanMeshArena&) = delete;
    VulkanMeshArena& operator=(const VulkanMeshArena&) = delete;

    [[nodiscard]] VulkanMeshAllocation allocate(VkDeviceSize size);
    void release(const VulkanMeshAllocation& allocation) noexcept;
    void upload(const VulkanMeshAllocation& allocation, std::span<const std::byte> vertices, VkDeviceSize indexOffset,
        std::span<const MeshLevel> levels);

    [[nodiscard]] VkBuffer buffer(std::size_t page) const noexcept;
    [[nodiscard]] std::size_t page_count() const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;

private:
    static constexpr VkDeviceSize Alignment = 32;
    static constexpr VkDeviceSize PageCapacity = 128ULL * 1024ULL * 1024ULL;

    [[nodiscard]] static VkDeviceSize aligned(VkDeviceSize size);
    [[nodiscard]] VulkanMeshAllocation allocate_page(VkDeviceSize size);

    VulkanDevice& device;
    std::vector<std::unique_ptr<VulkanMeshPage>> pages;
    std::uint64_t currentRevision = 0;
};

}
