#include "Render/Vulkan/Mesh/page.hpp"

#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"

#include <algorithm>
#include <stdexcept>

namespace Northrend {

VulkanMeshPage::VulkanMeshPage(VulkanDevice& device, const VkDeviceSize capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("Vulkan mesh page capacity must be positive");
    }

    this->buffer = std::make_unique<VulkanBuffer>(device, capacity,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->freeRanges.emplace_back(0, capacity);
}

VulkanMeshPage::~VulkanMeshPage() = default;

std::optional<VkDeviceSize> VulkanMeshPage::allocate(const VkDeviceSize size) {
    for (std::size_t range = 0; range < this->freeRanges.size(); ++range) {
        auto& [offset, available] = this->freeRanges[range];

        if (available < size) {
            continue;
        }

        const VkDeviceSize allocationOffset = offset;
        offset += size;
        available -= size;

        if (available == 0) {
            this->freeRanges.erase(this->freeRanges.begin() + static_cast<std::ptrdiff_t>(range));
        }

        return allocationOffset;
    }

    return std::nullopt;
}

void VulkanMeshPage::release(const VkDeviceSize offset, const VkDeviceSize size) noexcept {
    if (size == 0 || offset >= this->buffer->size() || size > this->buffer->size() - offset) {
        return;
    }

    try {
        this->freeRanges.emplace_back(offset, size);
        this->merge_free_ranges();
    } catch (...) {
        // Releasing GPU memory must remain safe during resource destruction.
    }
}

VkBuffer VulkanMeshPage::handle() const noexcept {
    return this->buffer->handle();
}

void VulkanMeshPage::merge_free_ranges() noexcept {
    std::ranges::sort(this->freeRanges, {}, &std::pair<VkDeviceSize, VkDeviceSize>::first);
    std::size_t output = 0;

    for (const auto& [offset, size] : this->freeRanges) {
        if (output != 0) {
            auto& [previousOffset, previousSize] = this->freeRanges[output - 1];

            if (offset <= previousOffset + previousSize) {
                previousSize = std::max(previousOffset + previousSize, offset + size) - previousOffset;
                continue;
            }
        }

        this->freeRanges[output++] = {offset, size};
    }

    this->freeRanges.resize(output);
}

}
