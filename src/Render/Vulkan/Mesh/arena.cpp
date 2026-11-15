#include "Render/Vulkan/Mesh/arena.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Mesh/page.hpp"
#include "Render/Vulkan/Transfer/segment.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMeshArena::VulkanMeshArena(VulkanDevice& device) noexcept : device(device) {}

VulkanMeshArena::~VulkanMeshArena() = default;

VulkanMeshAllocation VulkanMeshArena::allocate(const VkDeviceSize size) {
    const VkDeviceSize allocationSize = aligned(size);

    for (std::size_t page = 0; page < this->pages.size(); ++page) {
        const std::optional<VkDeviceSize> offset = this->pages[page]->allocate(allocationSize);

        if (!offset) {
            continue;
        }

        return VulkanMeshAllocation {.page = page, .offset = *offset, .size = allocationSize};
    }

    return this->allocate_page(allocationSize);
}

void VulkanMeshArena::release(const VulkanMeshAllocation& allocation) noexcept {
    if (allocation.page >= this->pages.size()) {
        return;
    }

    this->pages[allocation.page]->release(allocation.offset, allocation.size);
}

void VulkanMeshArena::upload(const VulkanMeshAllocation& allocation, const std::span<const std::byte> vertices,
    const VkDeviceSize indexOffset, const std::span<const MeshLevel> levels) {
    if (vertices.empty() || levels.empty() || indexOffset < vertices.size_bytes()) {
        throw std::invalid_argument("Vulkan mesh arena upload is invalid");
    }

    std::vector<VulkanTransferSegment> segments;
    segments.reserve(levels.size() + 1);
    segments.push_back(VulkanTransferSegment {.data = vertices, .offset = 0});
    VkDeviceSize size = indexOffset;

    for (const MeshLevel& level : levels) {
        if (size > std::numeric_limits<VkDeviceSize>::max() - level.indices.size_bytes()) {
            throw std::overflow_error("Vulkan mesh arena upload size overflow");
        }

        if (!level.indices.empty()) {
            segments.push_back(VulkanTransferSegment {.data = level.indices, .offset = size});
        }

        size += level.indices.size_bytes();
    }

    if (allocation.page >= this->pages.size() || size > allocation.size) {
        throw std::out_of_range("Vulkan mesh arena upload exceeds its allocation");
    }

    this->device.upload_buffer(this->pages[allocation.page]->handle(), allocation.offset, size, segments);
}

VkBuffer VulkanMeshArena::buffer(const std::size_t page) const noexcept {
    return page < this->pages.size() ? this->pages[page]->handle() : VK_NULL_HANDLE;
}

std::size_t VulkanMeshArena::page_count() const noexcept {
    return this->pages.size();
}

std::uint64_t VulkanMeshArena::revision() const noexcept {
    return this->currentRevision;
}

VkDeviceSize VulkanMeshArena::aligned(const VkDeviceSize size) {
    if (size == 0) {
        throw std::invalid_argument("Vulkan mesh allocation size must be positive");
    }

    if (size > std::numeric_limits<VkDeviceSize>::max() - (Alignment - 1)) {
        throw std::overflow_error("Vulkan mesh allocation size overflow");
    }

    const VkDeviceSize remainder = size % Alignment;
    return remainder == 0 ? size : size + Alignment - remainder;
}

VulkanMeshAllocation VulkanMeshArena::allocate_page(const VkDeviceSize size) {
    const VkDeviceSize capacity = std::max(PageCapacity, size);
    const std::size_t page = this->pages.size();
    auto added = std::make_unique<VulkanMeshPage>(this->device, capacity);
    const std::optional<VkDeviceSize> offset = added->allocate(size);

    if (!offset) {
        throw std::runtime_error("Failed to allocate from a new Vulkan mesh page");
    }

    this->pages.push_back(std::move(added));
    ++this->currentRevision;
    return VulkanMeshAllocation {.page = page, .offset = *offset, .size = size};
}

}
