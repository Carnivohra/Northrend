#include "Render/Vulkan/commands.hpp"

#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/target.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Mesh/mesh.hpp"
#include "Render/Vulkan/Pipeline/mesh.hpp"
#include "Render/frame.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Northrend {

VulkanCommandContext::VulkanCommandContext(VulkanDevice& device) : device(device) {
    const VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = this->device.graphics_family()
    };
    const VkResult result = vkCreateCommandPool(this->device.handle(), &createInfo, nullptr, &this->pool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan command pool: " + std::to_string(result));
    }
}

VulkanCommandContext::~VulkanCommandContext() {
    this->release();

    if (this->pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(this->device.handle(), this->pool, nullptr);
    }
}

void VulkanCommandContext::allocate(std::size_t count) {
    this->release();

    std::vector<VkCommandBuffer> allocated(count);
    const VkCommandBufferAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = this->pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<std::uint32_t>(allocated.size())
    };
    const VkResult result = vkAllocateCommandBuffers(this->device.handle(), &allocateInfo, allocated.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan command buffers: " + std::to_string(result));
    }

    this->buffers = std::move(allocated);
    this->recordedInstanceRevisions.assign(count, 0);
    this->recordedMeshRevisions.assign(count, 0);
    this->recorded.assign(count, false);
}

void VulkanCommandContext::release() noexcept {
    this->recorded.clear();
    this->recordedMeshRevisions.clear();
    this->recordedInstanceRevisions.clear();

    if (this->buffers.empty()) {
        return;
    }

    vkFreeCommandBuffers(
        this->device.handle(), this->pool, static_cast<std::uint32_t>(this->buffers.size()), this->buffers.data());
    this->buffers.clear();
}

void VulkanCommandContext::record(std::size_t imageIndex, const RenderFrame& renderFrame,
    const VulkanRenderTarget& target, VulkanMeshPipeline& pipeline, const VulkanMeshStore& meshes,
    const VkClearColorValue& clearColor) {
    const std::uint64_t meshRevision = meshes.revision();

    if (this->recorded[imageIndex] && this->recordedInstanceRevisions[imageIndex] == renderFrame.instancesRevision &&
        this->recordedMeshRevisions[imageIndex] == meshRevision) {
        return;
    }

    const VkCommandBuffer commandBuffer = this->buffers[imageIndex];
    VkResult result = vkResetCommandBuffer(commandBuffer, 0);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset a Vulkan command buffer: " + std::to_string(result));
    }

    const VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr, .flags = 0, .pInheritanceInfo = nullptr
    };
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin a Vulkan command buffer: " + std::to_string(result));
    }

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = VkClearDepthStencilValue {.depth = 1.0F, .stencil = 0};

    const VkRenderPassBeginInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = target.render_pass(),
        .framebuffer = target.framebuffer(imageIndex),
        .renderArea = VkRect2D {.offset = VkOffset2D {.x = 0, .y = 0}, .extent = target.extent()},
        .clearValueCount = static_cast<std::uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    pipeline.bind(commandBuffer, imageIndex);

    for (const RenderInstance& instance : renderFrame.instances) {
        const VulkanMesh* mesh = meshes.find(instance.mesh);

        if (!mesh) {
            continue;
        }

        pipeline.push_transform(commandBuffer, instance.transform);
        mesh->draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
    result = vkEndCommandBuffer(commandBuffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to record a Vulkan command buffer: " + std::to_string(result));
    }

    this->recordedInstanceRevisions[imageIndex] = renderFrame.instancesRevision;
    this->recordedMeshRevisions[imageIndex] = meshRevision;
    this->recorded[imageIndex] = true;
}

VkCommandBuffer VulkanCommandContext::handle(std::size_t imageIndex) const noexcept {
    return this->buffers[imageIndex];
}

}
