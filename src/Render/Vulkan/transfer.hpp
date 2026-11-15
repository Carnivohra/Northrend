#pragma once

#include "Render/Vulkan/Transfer/segment.hpp"

#include <volk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanTransferPage;

class VulkanTransferContext final {
public:
    VulkanTransferContext(VulkanDevice& device, std::size_t batchCount);
    ~VulkanTransferContext();

    VulkanTransferContext(const VulkanTransferContext&) = delete;
    VulkanTransferContext& operator=(const VulkanTransferContext&) = delete;

    void queue_buffer(VkBuffer destination, VkDeviceSize destinationOffset, VkDeviceSize size,
        std::span<const VulkanTransferSegment> segments);
    void queue_texture(VkImage destination, std::span<const VkBufferImageCopy> regions, std::uint32_t mipLevels,
        std::uint32_t layers, VkDeviceSize size, std::span<const VulkanTransferSegment> segments);
    [[nodiscard]] VkCommandBuffer flush(std::size_t batch, std::uint64_t submissionSerial);
    void collect(std::uint64_t completedSerial) noexcept;
    [[nodiscard]] bool pending() const noexcept;

private:
    struct Allocation final {
        std::size_t page;
        VkDeviceSize offset;
        VkDeviceSize size;
    };

    struct BufferUpload final {
        Allocation source;
        VkBuffer destination;
        VkDeviceSize destinationOffset;
        VkDeviceSize size;
    };

    struct TextureUpload final {
        Allocation source;
        VkImage destination;
        std::vector<VkBufferImageCopy> regions;
        std::uint32_t mipLevels;
        std::uint32_t layers;
    };

    struct RetiredAllocation final {
        std::uint64_t serial;
        Allocation allocation;
    };

    static constexpr VkDeviceSize Alignment = 256;
    static constexpr VkDeviceSize PageCapacity = 64ULL * 1024ULL * 1024ULL;

    [[nodiscard]] static VkDeviceSize aligned(VkDeviceSize size);
    [[nodiscard]] Allocation allocate(VkDeviceSize size);
    void release(const Allocation& allocation) noexcept;
    void write(const Allocation& allocation, std::span<const VulkanTransferSegment> segments);
    void destroy() noexcept;

    VulkanDevice& device;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<std::unique_ptr<VulkanTransferPage>> pages;
    std::vector<BufferUpload> buffers;
    std::vector<TextureUpload> textures;
    std::vector<RetiredAllocation> retired;
};

}
