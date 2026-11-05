#include "Render/Vulkan/Material/store.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Texture/store.hpp"
#include "Render/Vulkan/Texture/texture.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanMaterialStore::VulkanMaterialStore(VulkanDevice& device, const VulkanTextureStore& textures) :
    device(device), textures(textures) {
    try {
        const VkDescriptorSetLayoutBinding binding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MaximumTextures,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };
        const VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = 1,
            .pBindings = &binding
        };
        VkResult result =
            vkCreateDescriptorSetLayout(this->device.handle(), &layoutInfo, nullptr, &this->descriptorLayout);

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create the Vulkan material descriptor layout: " + std::to_string(result));
        }

        const VkDescriptorPoolSize poolSize {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = MaximumMaterials * MaximumTextures
        };
        const VkDescriptorPoolCreateInfo poolInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = MaximumMaterials,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize
        };
        result = vkCreateDescriptorPool(this->device.handle(), &poolInfo, nullptr, &this->descriptorPool);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the Vulkan material descriptor pool: " + std::to_string(result));
        }

        const VkSamplerCreateInfo samplerInfo {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0F,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1.0F,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0F,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        };
        result = vkCreateSampler(this->device.handle(), &samplerInfo, nullptr, &this->sampler);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the Vulkan material sampler: " + std::to_string(result));
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanMaterialStore::~VulkanMaterialStore() {
    this->destroy();
}

void VulkanMaterialStore::store(const MaterialHandle handle, const MaterialData& data) {
    if (!handle || data.textures().empty() || data.textures().size() > MaximumTextures) {
        throw std::invalid_argument("Vulkan material data is invalid");
    }

    const std::uint64_t rawIndex = handle.id() - 1;

    if (rawIndex >= MaximumMaterials || rawIndex >= std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("Material handle exceeds the Vulkan material capacity");
    }

    const std::size_t index = static_cast<std::size_t>(rawIndex);

    if (this->materials.size() <= index) {
        this->materials.resize(index + 1, VK_NULL_HANDLE);
    }

    if (this->materials[index] != VK_NULL_HANDLE) {
        throw std::invalid_argument("A Vulkan material already exists for this handle");
    }

    const VkDescriptorSetAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = this->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &this->descriptorLayout
    };
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    const VkResult result = vkAllocateDescriptorSets(this->device.handle(), &allocateInfo, &descriptorSet);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a Vulkan material descriptor set: " + std::to_string(result));
    }

    try {
        std::array<VkDescriptorImageInfo, MaximumTextures> images;
        const VulkanTexture* fallback = this->textures.find(data.textures().front());

        if (!fallback) {
            throw std::runtime_error("Vulkan material references a missing texture");
        }

        images.fill(VkDescriptorImageInfo {
            .sampler = this->sampler,
            .imageView = fallback->view(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });

        for (std::size_t texture = 0; texture < data.textures().size(); ++texture) {
            const VulkanTexture* image = this->textures.find(data.textures()[texture]);

            if (!image) {
                throw std::runtime_error("Vulkan material references a missing texture");
            }

            images[texture].imageView = image->view();
        }

        const VkWriteDescriptorSet write {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = MaximumTextures,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = images.data(),
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };
        vkUpdateDescriptorSets(this->device.handle(), 1, &write, 0, nullptr);
    } catch (...) {
        vkFreeDescriptorSets(this->device.handle(), this->descriptorPool, 1, &descriptorSet);
        throw;
    }

    this->materials[index] = descriptorSet;
}

void VulkanMaterialStore::erase(const MaterialHandle handle) noexcept {
    if (!handle) {
        return;
    }

    const std::uint64_t index = handle.id() - 1;

    if (index >= this->materials.size() || this->materials[static_cast<std::size_t>(index)] == VK_NULL_HANDLE) {
        return;
    }

    this->device.wait_idle();
    VkDescriptorSet& material = this->materials[static_cast<std::size_t>(index)];
    vkFreeDescriptorSets(this->device.handle(), this->descriptorPool, 1, &material);
    material = VK_NULL_HANDLE;
}

void VulkanMaterialStore::bind(const VkCommandBuffer commandBuffer, const VkPipelineLayout pipelineLayout,
    const MaterialHandle handle) const noexcept {
    if (!handle || handle.id() - 1 >= this->materials.size()) {
        return;
    }

    const VkDescriptorSet material = this->materials[static_cast<std::size_t>(handle.id() - 1)];

    if (material != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &material, 0, nullptr);
    }
}

VkDescriptorSetLayout VulkanMaterialStore::descriptor_layout() const noexcept {
    return this->descriptorLayout;
}

void VulkanMaterialStore::destroy() noexcept {
    if (this->sampler != VK_NULL_HANDLE) {
        vkDestroySampler(this->device.handle(), this->sampler, nullptr);
        this->sampler = VK_NULL_HANDLE;
    }

    if (this->descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(this->device.handle(), this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;
    }

    if (this->descriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(this->device.handle(), this->descriptorLayout, nullptr);
        this->descriptorLayout = VK_NULL_HANDLE;
    }
}

}
