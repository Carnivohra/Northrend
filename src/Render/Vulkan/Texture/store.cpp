#include "Render/Vulkan/Texture/store.hpp"

#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Texture/texture.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanTextureStore::VulkanTextureStore(VulkanDevice& device) noexcept : device(device) {}

VulkanTextureStore::~VulkanTextureStore() = default;

void VulkanTextureStore::store(const TextureHandle handle, const TextureData& data) {
    if (!handle) {
        throw std::invalid_argument("Cannot store a Vulkan texture without a handle");
    }

    const std::uint64_t rawIndex = handle.id() - 1;

    if (rawIndex >= std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("Texture handle exceeds the platform index range");
    }

    const std::size_t index = static_cast<std::size_t>(rawIndex);

    if (this->textures.size() <= index) {
        this->textures.resize(index + 1);
    }

    if (this->textures[index]) {
        throw std::invalid_argument("A Vulkan texture already exists for this handle");
    }

    std::vector<VkBufferImageCopy> regions;
    regions.reserve(data.mip_count());
    VkDeviceSize uploadSize = 0;

    for (std::size_t mip = 0; mip < data.mip_count(); ++mip) {
        const VkDeviceSize remainder = uploadSize % UploadAlignment;

        if (remainder != 0) {
            uploadSize += UploadAlignment - remainder;
        }

        const std::span<const std::byte> bytes = data.mip(mip);

        if (bytes.size_bytes() > std::numeric_limits<VkDeviceSize>::max() - uploadSize) {
            throw std::overflow_error("Vulkan texture upload size overflow");
        }

        regions.push_back(VkBufferImageCopy {
            .bufferOffset = uploadSize,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                VkImageSubresourceLayers {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = static_cast<std::uint32_t>(mip),
                    .baseArrayLayer = 0,
                    .layerCount = data.layers()
                },
            .imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0},
            .imageExtent = VkExtent3D {.width = data.mip_width(mip), .height = data.mip_height(mip), .depth = 1}
        });
        uploadSize += bytes.size_bytes();
    }

    if (!this->staging || this->staging->size() < uploadSize) {
        this->grow_staging(uploadSize);
    }

    for (std::size_t mip = 0; mip < regions.size(); ++mip) {
        this->staging->write(data.mip(mip), regions[mip].bufferOffset);
    }

    auto texture = std::make_unique<VulkanTexture>(
        this->device, data.format(), data.width(), data.height(), data.layers(), data.mip_count(), data.array());
    this->device.copy_texture(this->staging->handle(), texture->handle(), regions,
        static_cast<std::uint32_t>(data.mip_count()), data.layers());
    this->textures[index] = std::move(texture);
}

void VulkanTextureStore::erase(const TextureHandle handle) noexcept {
    if (!handle) {
        return;
    }

    const std::uint64_t index = handle.id() - 1;

    if (index >= this->textures.size() || !this->textures[static_cast<std::size_t>(index)]) {
        return;
    }

    this->device.wait_idle();
    this->textures[static_cast<std::size_t>(index)].reset();
}

void VulkanTextureStore::erase(const std::span<const TextureHandle> handles) noexcept {
    const bool containsTexture = std::ranges::any_of(handles, [this](const TextureHandle handle) {
        const std::uint64_t index = handle ? handle.id() - 1 : this->textures.size();
        return index < this->textures.size() && this->textures[static_cast<std::size_t>(index)];
    });

    if (!containsTexture) {
        return;
    }

    this->device.wait_idle();

    for (const TextureHandle handle : handles) {
        if (!handle) {
            continue;
        }

        const std::uint64_t index = handle.id() - 1;

        if (index < this->textures.size()) {
            this->textures[static_cast<std::size_t>(index)].reset();
        }
    }
}

const VulkanTexture* VulkanTextureStore::find(const TextureHandle handle) const noexcept {
    if (!handle) {
        return nullptr;
    }

    const std::uint64_t index = handle.id() - 1;
    return index < this->textures.size() ? this->textures[static_cast<std::size_t>(index)].get() : nullptr;
}

void VulkanTextureStore::grow_staging(const VkDeviceSize required) {
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

}
