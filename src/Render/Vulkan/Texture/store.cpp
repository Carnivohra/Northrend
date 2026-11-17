#include "Render/Vulkan/Texture/store.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Transfer/segment.hpp"
#include "Render/Vulkan/Texture/texture.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include "Debug/interval.hpp"

#include <iomanip>
#include <iostream>
#endif

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Northrend {

VulkanTextureStore::VulkanTextureStore(VulkanDevice& device) noexcept : device(device) {}

VulkanTextureStore::~VulkanTextureStore() = default;

void VulkanTextureStore::store(const TextureHandle handle, const TextureData& data) {
    const std::size_t index = this->prepare(handle);

    std::vector<VkBufferImageCopy> regions;
    regions.reserve(data.mip_count());
    std::vector<VulkanTransferSegment> segments;
    segments.reserve(data.mip_count());
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
        segments.push_back(VulkanTransferSegment {.data = bytes, .offset = uploadSize});
        uploadSize += bytes.size_bytes();
    }

    auto texture = std::make_unique<VulkanTexture>(
        this->device, data.format(), data.width(), data.height(), data.layers(), data.mip_count(), data.array());
    this->device.upload_texture(
        texture->handle(), regions, static_cast<std::uint32_t>(data.mip_count()), data.layers(), uploadSize, segments);
#ifdef NORTHREND_GPU_PROFILING
    this->record(*texture);
#endif
    this->textures[index] = std::move(texture);
}

VulkanTexture& VulkanTextureStore::create(const TextureHandle handle, const TextureFormat format,
    const std::uint32_t width, const std::uint32_t height, const std::uint32_t layers, const std::size_t mipCount,
    const VkImageUsageFlags usage) {
    const std::size_t index = this->prepare(handle);
    auto texture = std::make_unique<VulkanTexture>(this->device, format, width, height, layers, mipCount, true, usage);
#ifdef NORTHREND_GPU_PROFILING
    this->record(*texture);
#endif
    VulkanTexture& result = *texture;
    this->textures[index] = std::move(texture);
    return result;
}

std::size_t VulkanTextureStore::prepare(const TextureHandle handle) {
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

    return index;
}

void VulkanTextureStore::erase(const TextureHandle handle, const std::uint64_t retirementSerial) {
    if (!handle) {
        return;
    }

    const std::uint64_t index = handle.id() - 1;

    if (index >= this->textures.size() || !this->textures[static_cast<std::size_t>(index)]) {
        return;
    }

    this->retired.emplace_back(retirementSerial, std::move(this->textures[static_cast<std::size_t>(index)]));
}

void VulkanTextureStore::erase(const std::span<const TextureHandle> handles, const std::uint64_t retirementSerial) {
    for (const TextureHandle handle : handles) {
        if (!handle) {
            continue;
        }

        const std::uint64_t index = handle.id() - 1;

        if (index < this->textures.size() && this->textures[static_cast<std::size_t>(index)]) {
            this->retired.emplace_back(retirementSerial, std::move(this->textures[static_cast<std::size_t>(index)]));
        }
    }
}

void VulkanTextureStore::collect(const std::uint64_t completedSerial) noexcept {
    std::erase_if(this->retired, [this, completedSerial](const auto& entry) {
        if (entry.first > completedSerial) {
            return false;
        }

#ifdef NORTHREND_GPU_PROFILING
        this->allocatedBytes -= entry.second->allocation_size();
#endif
        return true;
    });
#ifdef NORTHREND_GPU_PROFILING
    this->report();
#endif
}

const VulkanTexture* VulkanTextureStore::find(const TextureHandle handle) const noexcept {
    if (!handle) {
        return nullptr;
    }

    const std::uint64_t index = handle.id() - 1;
    return index < this->textures.size() ? this->textures[static_cast<std::size_t>(index)].get() : nullptr;
}

#ifdef NORTHREND_GPU_PROFILING
void VulkanTextureStore::record(const VulkanTexture& texture) noexcept {
    this->allocatedBytes += texture.allocation_size();
    this->peakAllocatedBytes = std::max(this->peakAllocatedBytes, this->allocatedBytes);
    this->imageCreationMilliseconds += texture.image_creation_time();
    this->memoryAllocationMilliseconds += texture.memory_allocation_time();
    ++this->createdImages;
    this->report();
}

void VulkanTextureStore::report() {
    static Interval textureInterval;

    if (!textureInterval.elapsed()) {
        return;
    }

    static constexpr double BytesPerMebibyte = 1024.0 * 1024.0;
    const std::ios::fmtflags flags = std::cout.flags();
    const std::streamsize precision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(3) << "Texture allocations: images=" << this->createdImages
              << " vkCreateImage_ms=" << this->imageCreationMilliseconds
              << " vkAllocateMemory_ms=" << this->memoryAllocationMilliseconds << " current_MiB="
              << static_cast<double>(this->allocatedBytes) / BytesPerMebibyte << " peak_MiB="
              << static_cast<double>(this->peakAllocatedBytes) / BytesPerMebibyte << '\n';
    std::cout.flags(flags);
    std::cout.precision(precision);
    this->createdImages = 0;
    this->imageCreationMilliseconds = 0.0;
    this->memoryAllocationMilliseconds = 0.0;
}
#endif

}
