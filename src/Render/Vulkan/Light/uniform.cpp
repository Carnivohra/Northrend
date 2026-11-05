#include "Render/Vulkan/Light/uniform.hpp"

#include "Render/Vulkan/Light/data.hpp"
#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/light.hpp"

#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

VulkanLightUniform::VulkanLightUniform(VulkanDevice& device, const std::size_t imageCount) : device(device) {
    try {
        this->create_layout();
        this->create_buffers(imageCount);
        this->create_descriptors();
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanLightUniform::~VulkanLightUniform() {
    this->destroy();
}

void VulkanLightUniform::update(const std::size_t imageIndex, const DirectionalLight& light) {
    VulkanLightBuffer& buffer = this->buffers[imageIndex];

    if (buffer.uploadRevision == light.revision) {
        return;
    }

    const float length = std::sqrt(light.direction.x * light.direction.x + light.direction.y * light.direction.y +
        light.direction.z * light.direction.z);
    const float inverseLength = length > 0.0F ? 1.0F / length : 0.0F;
    const VulkanLightData data {
        .directionAmbient =
            {light.direction.x * inverseLength, light.direction.y * inverseLength, light.direction.z * inverseLength,
                light.ambient},
        .colorIntensity = {light.color.x, light.color.y, light.color.z, light.intensity}
    };
    std::memcpy(buffer.mapped, &data, sizeof(data));
    buffer.uploadRevision = light.revision;
}

void VulkanLightUniform::bind(const VkCommandBuffer commandBuffer, const VkPipelineLayout pipelineLayout,
    const std::size_t imageIndex) const noexcept {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
        &this->descriptorSets[imageIndex], 0, nullptr);
}

VkDescriptorSetLayout VulkanLightUniform::descriptor_layout() const noexcept {
    return this->descriptorLayout;
}

void VulkanLightUniform::create_layout() {
    const VkDescriptorSetLayoutBinding lightBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };
    const VkDescriptorSetLayoutCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &lightBinding
    };
    const VkResult result =
        vkCreateDescriptorSetLayout(this->device.handle(), &createInfo, nullptr, &this->descriptorLayout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan light descriptor layout: " + std::to_string(result));
    }
}

void VulkanLightUniform::create_buffers(const std::size_t imageCount) {
    this->buffers.resize(imageCount);

    for (VulkanLightBuffer& buffer : this->buffers) {
        buffer.buffer =
            std::make_unique<VulkanBuffer>(this->device, sizeof(VulkanLightData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        buffer.mapped = buffer.buffer->map();
    }
}

void VulkanLightUniform::create_descriptors() {
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
        throw std::runtime_error("Failed to create the Vulkan light descriptor pool: " + std::to_string(result));
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
        throw std::runtime_error("Failed to allocate Vulkan light descriptors: " + std::to_string(result));
    }

    for (std::size_t index = 0; index < this->buffers.size(); ++index) {
        const VkDescriptorBufferInfo bufferInfo {
            .buffer = this->buffers[index].buffer->handle(), .offset = 0, .range = sizeof(VulkanLightData)
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

void VulkanLightUniform::destroy() noexcept {
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
