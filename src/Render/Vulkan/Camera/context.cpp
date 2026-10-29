#include "Render/Vulkan/Camera/context.hpp"

#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/camera.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanCameraContext::VulkanCameraContext(VulkanDevice& device, std::size_t imageCount) : device(device) {
    try {
        this->create_layout();
        this->create_buffers(imageCount);
        this->create_descriptors();
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanCameraContext::~VulkanCameraContext() {
    this->destroy();
}

void VulkanCameraContext::update(
    std::size_t imageIndex, const Camera& camera, const Matrix4& view, std::uint64_t revision, VkExtent2D extent) {
    const bool extentChanged = extent.width != this->sourceExtent.width || extent.height != this->sourceExtent.height;

    if (!this->initialized || revision != this->sourceRevision || extentChanged) {
        Matrix4 projection = camera.projection(static_cast<float>(extent.width) / static_cast<float>(extent.height));

        // Vulkan framebuffer coordinates use the opposite vertical direction.
        projection.values[5] *= -1.0F;

        this->matrix = projection * view;
        this->sourceExtent = extent;
        this->sourceRevision = revision;
        ++this->uploadRevision;
        this->initialized = true;
    }

    VulkanCameraBuffer& buffer = this->buffers[imageIndex];

    if (buffer.uploadRevision == this->uploadRevision) {
        return;
    }

    std::memcpy(buffer.mapped, this->matrix.values.data(), sizeof(this->matrix));
    buffer.uploadRevision = this->uploadRevision;
}

void VulkanCameraContext::bind(
    VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, std::size_t imageIndex) const noexcept {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
        &this->descriptorSets[imageIndex], 0, nullptr);
}

VkDescriptorSetLayout VulkanCameraContext::descriptor_layout() const noexcept {
    return this->descriptorLayout;
}

void VulkanCameraContext::create_layout() {
    const VkDescriptorSetLayoutBinding cameraBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    const VkDescriptorSetLayoutCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &cameraBinding
    };
    const VkResult result =
        vkCreateDescriptorSetLayout(this->device.handle(), &createInfo, nullptr, &this->descriptorLayout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan camera descriptor layout: " + std::to_string(result));
    }
}

void VulkanCameraContext::create_buffers(std::size_t imageCount) {
    this->buffers.resize(imageCount);

    for (VulkanCameraBuffer& buffer : this->buffers) {
        buffer.buffer =
            std::make_unique<VulkanBuffer>(this->device, sizeof(Matrix4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        buffer.mapped = buffer.buffer->map();
    }
}

void VulkanCameraContext::create_descriptors() {
    const std::uint32_t descriptorCount = static_cast<std::uint32_t>(this->buffers.size());
    const VkDescriptorPoolSize poolSize {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = descriptorCount};
    const VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = descriptorCount,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    VkResult result = vkCreateDescriptorPool(this->device.handle(), &poolInfo, nullptr, &this->descriptorPool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan camera descriptor pool: " + std::to_string(result));
    }

    const std::vector<VkDescriptorSetLayout> layouts(this->buffers.size(), this->descriptorLayout);
    this->descriptorSets.resize(this->buffers.size());

    const VkDescriptorSetAllocateInfo allocationInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = this->descriptorPool,
        .descriptorSetCount = descriptorCount,
        .pSetLayouts = layouts.data()
    };
    result = vkAllocateDescriptorSets(this->device.handle(), &allocationInfo, this->descriptorSets.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan camera descriptors: " + std::to_string(result));
    }

    for (std::size_t index = 0; index < this->buffers.size(); ++index) {
        const VkDescriptorBufferInfo bufferInfo {
            .buffer = this->buffers[index].buffer->handle(), .offset = 0, .range = sizeof(Matrix4)
        };
        const VkWriteDescriptorSet write {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = this->descriptorSets[index],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfo,
            .pTexelBufferView = nullptr
        };
        vkUpdateDescriptorSets(this->device.handle(), 1, &write, 0, nullptr);
    }
}

void VulkanCameraContext::destroy() noexcept {
    if (this->descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(this->device.handle(), this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;
    }
    this->descriptorSets.clear();
    this->buffers.clear();

    if (this->descriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(this->device.handle(), this->descriptorLayout, nullptr);
        this->descriptorLayout = VK_NULL_HANDLE;
    }
}

}
