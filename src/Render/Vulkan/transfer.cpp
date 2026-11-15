#include "Render/Vulkan/transfer.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Transfer/page.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

VulkanTransferContext::VulkanTransferContext(VulkanDevice& device, const std::size_t batchCount) : device(device) {
    if (batchCount == 0 || batchCount > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Vulkan transfer batch count is invalid");
    }

    try {
        const VkCommandPoolCreateInfo poolInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = this->device.transfer_family()
        };
        VkResult result = vkCreateCommandPool(this->device.handle(), &poolInfo, nullptr, &this->pool);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the Vulkan transfer command pool: " + std::to_string(result));
        }

        this->commandBuffers.resize(batchCount);
        const VkCommandBufferAllocateInfo allocationInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = this->pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(this->commandBuffers.size())
        };
        result = vkAllocateCommandBuffers(this->device.handle(), &allocationInfo, this->commandBuffers.data());

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to allocate Vulkan transfer command buffers: " + std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanTransferContext::~VulkanTransferContext() {
    this->destroy();
}

void VulkanTransferContext::queue_buffer(const VkBuffer destination, const VkDeviceSize destinationOffset,
    const VkDeviceSize size, const std::span<const VulkanTransferSegment> segments) {
    if (destination == VK_NULL_HANDLE || size == 0 || segments.empty()) {
        throw std::invalid_argument("Vulkan buffer transfer is invalid");
    }

    const Allocation allocation = this->allocate(size);

    try {
        this->write(allocation, segments);
        this->buffers.push_back(BufferUpload {
            .source = allocation,
            .destination = destination,
            .destinationOffset = destinationOffset,
            .size = size
        });
    } catch (...) {
        this->release(allocation);
        throw;
    }
}

void VulkanTransferContext::queue_texture(const VkImage destination,
    const std::span<const VkBufferImageCopy> regions, const std::uint32_t mipLevels, const std::uint32_t layers,
    const VkDeviceSize size, const std::span<const VulkanTransferSegment> segments) {
    if (destination == VK_NULL_HANDLE || regions.empty() || mipLevels == 0 || layers == 0 || size == 0 ||
        segments.empty()) {
        throw std::invalid_argument("Vulkan texture transfer is invalid");
    }

    std::vector<VkBufferImageCopy> storedRegions(regions.begin(), regions.end());
    const Allocation allocation = this->allocate(size);

    try {
        for (VkBufferImageCopy& region : storedRegions) {
            if (region.bufferOffset > std::numeric_limits<VkDeviceSize>::max() - allocation.offset) {
                throw std::overflow_error("Vulkan texture transfer offset overflow");
            }

            region.bufferOffset += allocation.offset;
        }

        this->write(allocation, segments);
        this->textures.push_back(TextureUpload {
            .source = allocation,
            .destination = destination,
            .regions = std::move(storedRegions),
            .mipLevels = mipLevels,
            .layers = layers
        });
    } catch (...) {
        this->release(allocation);
        throw;
    }
}

VkCommandBuffer VulkanTransferContext::flush(const std::size_t batch, const std::uint64_t submissionSerial) {
    if (!this->pending()) {
        return VK_NULL_HANDLE;
    }

    if (batch >= this->commandBuffers.size()) {
        throw std::out_of_range("Vulkan transfer batch is outside the command buffer range");
    }

    this->retired.reserve(this->retired.size() + this->buffers.size() + this->textures.size());
    const VkCommandBuffer commandBuffer = this->commandBuffers[batch];
    VkResult result = vkResetCommandBuffer(commandBuffer, 0);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset a Vulkan transfer command buffer: " + std::to_string(result));
    }

    const VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin a Vulkan transfer command buffer: " + std::to_string(result));
    }

    const bool separateQueue = this->device.dedicated_transfer_queue();
    std::vector<VkBufferMemoryBarrier> bufferBarriers;

    if (!separateQueue) {
        bufferBarriers.reserve(this->buffers.size());
    }

    for (const BufferUpload& upload : this->buffers) {
        const VkBufferCopy region {
            .srcOffset = upload.source.offset,
            .dstOffset = upload.destinationOffset,
            .size = upload.size
        };
        vkCmdCopyBuffer(
            commandBuffer, this->pages[upload.source.page]->handle(), upload.destination, 1, &region);
        if (!separateQueue) {
            bufferBarriers.push_back(VkBufferMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
                    VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = upload.destination,
                .offset = upload.destinationOffset,
                .size = upload.size
            });
        }
    }

    if (!bufferBarriers.empty()) {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr,
            static_cast<std::uint32_t>(bufferBarriers.size()), bufferBarriers.data(), 0, nullptr);
    }

    for (const TextureUpload& upload : this->textures) {
        const VkImageSubresourceRange range {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = upload.mipLevels,
            .baseArrayLayer = 0,
            .layerCount = upload.layers
        };
        const VkImageMemoryBarrier transferBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = upload.destination,
            .subresourceRange = range
        };
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &transferBarrier);
        vkCmdCopyBufferToImage(commandBuffer, this->pages[upload.source.page]->handle(), upload.destination,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<std::uint32_t>(upload.regions.size()),
            upload.regions.data());

        const VkImageMemoryBarrier sampleBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask =
                separateQueue ? VkAccessFlags {} : static_cast<VkAccessFlags>(VK_ACCESS_SHADER_READ_BIT),
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = upload.destination,
            .subresourceRange = range
        };
        const VkPipelineStageFlags destinationStage =
            separateQueue ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationStage, 0, 0, nullptr, 0,
            nullptr, 1, &sampleBarrier);
    }

    result = vkEndCommandBuffer(commandBuffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to end a Vulkan transfer command buffer: " + std::to_string(result));
    }

    for (const BufferUpload& upload : this->buffers) {
        this->retired.push_back(RetiredAllocation {.serial = submissionSerial, .allocation = upload.source});
    }

    for (const TextureUpload& upload : this->textures) {
        this->retired.push_back(RetiredAllocation {.serial = submissionSerial, .allocation = upload.source});
    }

    this->buffers.clear();
    this->textures.clear();
    return commandBuffer;
}

void VulkanTransferContext::collect(const std::uint64_t completedSerial) noexcept {
    std::erase_if(this->retired, [this, completedSerial](const RetiredAllocation& entry) {
        if (entry.serial > completedSerial) {
            return false;
        }

        this->release(entry.allocation);
        return true;
    });
}

bool VulkanTransferContext::pending() const noexcept {
    return !this->buffers.empty() || !this->textures.empty();
}

VkDeviceSize VulkanTransferContext::aligned(const VkDeviceSize size) {
    if (size == 0 || size > std::numeric_limits<VkDeviceSize>::max() - (Alignment - 1)) {
        throw std::overflow_error("Vulkan transfer allocation size overflow");
    }

    const VkDeviceSize remainder = size % Alignment;
    return remainder == 0 ? size : size + Alignment - remainder;
}

VulkanTransferContext::Allocation VulkanTransferContext::allocate(const VkDeviceSize size) {
    const VkDeviceSize allocationSize = aligned(size);

    for (std::size_t page = 0; page < this->pages.size(); ++page) {
        const std::optional<VkDeviceSize> offset = this->pages[page]->allocate(allocationSize);

        if (offset) {
            return Allocation {.page = page, .offset = *offset, .size = allocationSize};
        }
    }

    const VkDeviceSize capacity = std::max(PageCapacity, allocationSize);
    const std::size_t page = this->pages.size();
    auto added = std::make_unique<VulkanTransferPage>(this->device, capacity);
    const std::optional<VkDeviceSize> offset = added->allocate(allocationSize);

    if (!offset) {
        throw std::runtime_error("Failed to allocate from a new Vulkan transfer page");
    }

    this->pages.push_back(std::move(added));
    return Allocation {.page = page, .offset = *offset, .size = allocationSize};
}

void VulkanTransferContext::release(const Allocation& allocation) noexcept {
    if (allocation.page < this->pages.size()) {
        this->pages[allocation.page]->release(allocation.offset, allocation.size);
    }
}

void VulkanTransferContext::write(
    const Allocation& allocation, const std::span<const VulkanTransferSegment> segments) {
    for (const VulkanTransferSegment& segment : segments) {
        if (segment.offset > allocation.size || segment.data.size_bytes() > allocation.size - segment.offset) {
            throw std::out_of_range("Vulkan transfer segment exceeds its allocation");
        }

        this->pages[allocation.page]->write(segment.data, allocation.offset + segment.offset);
    }
}

void VulkanTransferContext::destroy() noexcept {
    this->buffers.clear();
    this->textures.clear();
    this->retired.clear();
    this->pages.clear();
    this->commandBuffers.clear();

    if (this->pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(this->device.handle(), this->pool, nullptr);
        this->pool = VK_NULL_HANDLE;
    }
}

}
