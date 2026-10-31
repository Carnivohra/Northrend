#include "Render/Vulkan/Mesh/arena.hpp"

#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMeshArena::VulkanMeshArena(VulkanDevice& device) noexcept : device(device) {}

VulkanMeshArena::~VulkanMeshArena() = default;

VkDeviceSize VulkanMeshArena::allocate(const VkDeviceSize size) {
    const VkDeviceSize allocationSize = aligned(size);

    for (std::size_t range = 0; range < this->freeRanges.size(); ++range) {
        auto& [offset, available] = this->freeRanges[range];

        if (available < allocationSize) {
            continue;
        }

        const VkDeviceSize allocationOffset = offset;
        offset += allocationSize;
        available -= allocationSize;

        if (available == 0) {
            this->freeRanges.erase(this->freeRanges.begin() + static_cast<std::ptrdiff_t>(range));
        }

        return allocationOffset;
    }

    this->grow(allocationSize);
    return this->allocate(allocationSize);
}

void VulkanMeshArena::release(const VkDeviceSize offset, const VkDeviceSize size) noexcept {
    if (size == 0 || !this->buffer || offset >= this->buffer->size()) {
        return;
    }

    if (size > std::numeric_limits<VkDeviceSize>::max() - (Alignment - 1)) {
        return;
    }

    const VkDeviceSize remainder = size % Alignment;
    const VkDeviceSize allocationSize = remainder == 0 ? size : size + Alignment - remainder;

    if (allocationSize > this->buffer->size() - offset) {
        return;
    }

    try {
        this->freeRanges.emplace_back(offset, allocationSize);
        this->merge_free_ranges();
    } catch (...) {
        // Releasing GPU memory must remain safe during resource destruction.
    }
}

void VulkanMeshArena::upload(const VkDeviceSize offset, const std::span<const std::byte> vertices,
    const VkDeviceSize indexOffset, const std::span<const std::byte> indices) {
    if (vertices.empty() || indices.empty() || indexOffset < vertices.size_bytes()) {
        throw std::invalid_argument("Vulkan mesh arena upload is invalid");
    }

    if (indexOffset > std::numeric_limits<VkDeviceSize>::max() - indices.size_bytes()) {
        throw std::overflow_error("Vulkan mesh arena upload size overflow");
    }

    const VkDeviceSize size = indexOffset + indices.size_bytes();

    if (!this->buffer || offset > this->buffer->size() || size > this->buffer->size() - offset) {
        throw std::out_of_range("Vulkan mesh arena upload exceeds its allocation");
    }

    if (!this->staging || this->staging->size() < size) {
        this->grow_staging(size);
    }

    this->staging->write(vertices);
    this->staging->write(indices, indexOffset);
    this->device.copy_buffer(this->staging->handle(), this->buffer->handle(), size, 0, offset);
}

VkBuffer VulkanMeshArena::handle() const noexcept {
    return this->buffer ? this->buffer->handle() : VK_NULL_HANDLE;
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

VkDeviceSize VulkanMeshArena::grown_capacity(const VkDeviceSize capacity, const VkDeviceSize required) {
    if (capacity > std::numeric_limits<VkDeviceSize>::max() - required) {
        throw std::overflow_error("Vulkan mesh arena capacity overflow");
    }

    const VkDeviceSize minimum = capacity + required;
    VkDeviceSize grown = capacity == 0 ? aligned(InitialCapacity) : capacity;

    while (grown < minimum) {
        if (grown > std::numeric_limits<VkDeviceSize>::max() / 2) {
            return minimum;
        }

        grown *= 2;
    }

    return grown;
}

void VulkanMeshArena::grow(const VkDeviceSize required) {
    const VkDeviceSize previousCapacity = this->buffer ? this->buffer->size() : 0;
    const VkDeviceSize capacity = grown_capacity(previousCapacity, required);
    this->freeRanges.reserve(this->freeRanges.size() + 1);
    auto expanded = std::make_unique<VulkanBuffer>(this->device, capacity,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (this->buffer) {
        this->device.wait_idle();
        this->device.copy_buffer(this->buffer->handle(), expanded->handle(), previousCapacity);
    }

    this->buffer = std::move(expanded);
    this->freeRanges.emplace_back(previousCapacity, capacity - previousCapacity);
    this->merge_free_ranges();
    ++this->currentRevision;
}

void VulkanMeshArena::grow_staging(const VkDeviceSize required) {
    VkDeviceSize capacity = this->staging ? this->staging->size() : InitialStagingCapacity;

    while (capacity < required) {
        if (capacity > std::numeric_limits<VkDeviceSize>::max() / 2) {
            capacity = required;
            break;
        }

        capacity *= 2;
    }

    this->staging = std::make_unique<VulkanBuffer>(this->device, capacity, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void VulkanMeshArena::merge_free_ranges() noexcept {
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
