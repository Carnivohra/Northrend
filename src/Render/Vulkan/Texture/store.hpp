#pragma once

#include "Render/Texture/data.hpp"
#include "Render/Texture/texture.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanTexture;

class VulkanTextureStore final {
public:
    explicit VulkanTextureStore(VulkanDevice& device) noexcept;
    ~VulkanTextureStore();

    VulkanTextureStore(const VulkanTextureStore&) = delete;
    VulkanTextureStore& operator=(const VulkanTextureStore&) = delete;

    void store(TextureHandle handle, const TextureData& data);
    [[nodiscard]] VulkanTexture& create(TextureHandle handle, TextureFormat format, std::uint32_t width,
        std::uint32_t height, std::uint32_t layers, std::size_t mipCount, VkImageUsageFlags usage);
    void erase(TextureHandle handle, std::uint64_t retirementSerial);
    void erase(std::span<const TextureHandle> handles, std::uint64_t retirementSerial);
    void collect(std::uint64_t completedSerial) noexcept;
    [[nodiscard]] const VulkanTexture* find(TextureHandle handle) const noexcept;

private:
    [[nodiscard]] std::size_t prepare(TextureHandle handle);
#ifdef NORTHREND_GPU_PROFILING
    void record(const VulkanTexture& texture) noexcept;
    void report();
#endif

    static constexpr VkDeviceSize UploadAlignment = 16;

    VulkanDevice& device;
    std::vector<std::unique_ptr<VulkanTexture>> textures;
    std::vector<std::pair<std::uint64_t, std::unique_ptr<VulkanTexture>>> retired;
#ifdef NORTHREND_GPU_PROFILING
    VkDeviceSize allocatedBytes = 0;
    VkDeviceSize peakAllocatedBytes = 0;
    double imageCreationMilliseconds = 0.0;
    double memoryAllocationMilliseconds = 0.0;
    std::uint64_t createdImages = 0;
#endif
};

}
