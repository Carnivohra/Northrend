#include "Render/Vulkan/Texture/blend.hpp"

#include "Render/Vulkan/Shader/Texture/blend.hpp"
#include "Render/Vulkan/Texture/store.hpp"
#include "Render/Vulkan/Texture/texture.hpp"
#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"

#ifdef NORTHREND_GPU_PROFILING
#include "Debug/interval.hpp"

#include <iostream>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanTextureBlend::VulkanTextureBlend(
    VulkanDevice& device, VulkanTextureStore& textures, const std::size_t batchCount) :
    device(device), textures(textures) {
    try {
        this->create_layout();
        this->create_descriptors();
        this->create_pipeline();
        this->create_commands(batchCount);
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanTextureBlend::~VulkanTextureBlend() {
    this->destroy();
}

void VulkanTextureBlend::store(const TextureHandle handle, const TextureBlendData& data) {
    if (data.textures().size() > MaximumTextures ||
        data.cells().size_bytes() > std::numeric_limits<VkDeviceSize>::max()) {
        throw std::invalid_argument("Vulkan texture blend data exceeds the supported capacity");
    }

    Job job;
    job.handle = handle;
    job.cells = std::make_unique<VulkanBuffer>(this->device, data.cells().size_bytes(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    job.cells->write(std::as_bytes(data.cells()));
    job.width = data.cell_size();
    job.height = data.cell_size();
    job.layers = static_cast<std::uint32_t>(data.cells().size());
    job.mipCount = std::bit_width(std::max(job.width, job.height));
    job.columns = data.columns();
    job.rows = data.rows();
    job.cellSize = data.cell_size();
    job.textureScale = data.texture_scale();

    VulkanTexture* output = nullptr;

    try {
        output =
            &this->textures.create(handle, TextureFormat::rgba8Unorm, job.width, job.height, job.layers, job.mipCount,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT);
        job.output = output->handle();
        this->write_descriptor(job, data.textures(), data.alpha(), output->view());
        this->jobs.push_back(std::move(job));
    } catch (...) {
        if (!this->jobs.empty() && this->jobs.back().output == (output ? output->handle() : VK_NULL_HANDLE)) {
            const VkDescriptorSet descriptor = this->jobs.back().descriptor;
            this->jobs.pop_back();

            if (descriptor != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(this->device.handle(), this->descriptorPool, 1, &descriptor);
            }
        } else if (job.descriptor != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(this->device.handle(), this->descriptorPool, 1, &job.descriptor);
        }

        if (output != nullptr) {
            this->textures.erase(handle, 0);
            this->textures.collect(0);
        }

        throw;
    }
}

void VulkanTextureBlend::cancel(const TextureHandle handle) noexcept {
    const auto job = std::find_if(
        this->jobs.begin(), this->jobs.end(), [handle](const Job& candidate) { return candidate.handle == handle; });

    if (job == this->jobs.end()) {
        return;
    }

    if (job->descriptor != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(this->device.handle(), this->descriptorPool, 1, &job->descriptor);
    }

    this->jobs.erase(job);
#ifdef NORTHREND_GPU_PROFILING
    ++this->cancelledJobs;
    this->report();
#endif
}

VkCommandBuffer VulkanTextureBlend::flush(const std::size_t batch, const std::uint64_t submissionSerial) {
    if (this->jobs.empty()) {
        return VK_NULL_HANDLE;
    }

    if (batch >= this->commandBuffers.size()) {
        throw std::out_of_range("Vulkan texture blend batch is outside the command buffer range");
    }

    const VkCommandBuffer commandBuffer = this->commandBuffers[batch];
    VkResult result = vkResetCommandBuffer(commandBuffer, 0);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset a Vulkan texture blend command buffer: " + std::to_string(result));
    }

    const VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin a Vulkan texture blend command buffer: " + std::to_string(result));
    }

    const std::size_t jobCount = std::min(this->jobs.size(), JobsPerFrame);

    for (std::size_t index = 0; index < jobCount; ++index) {
        this->record(commandBuffer, this->jobs[index]);
    }

    result = vkEndCommandBuffer(commandBuffer);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to end a Vulkan texture blend command buffer: " + std::to_string(result));
    }

    this->retired.reserve(this->retired.size() + jobCount);

    for (std::size_t index = 0; index < jobCount; ++index) {
        this->retired.emplace_back(submissionSerial, std::move(this->jobs.front()));
        this->jobs.pop_front();
    }

#ifdef NORTHREND_GPU_PROFILING
    this->submittedJobs += jobCount;
    this->report();
#endif

    return commandBuffer;
}

void VulkanTextureBlend::collect(const std::uint64_t completedSerial) noexcept {
#ifdef NORTHREND_GPU_PROFILING
    std::size_t completed = 0;
    const auto collect = [this, completedSerial, &completed](const auto& entry) {
#else
    const auto collect = [this, completedSerial](const auto& entry) {
#endif
        if (entry.first > completedSerial) {
            return false;
        }

        const VkDescriptorSet descriptor = entry.second.descriptor;

        if (descriptor != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(this->device.handle(), this->descriptorPool, 1, &descriptor);
        }

#ifdef NORTHREND_GPU_PROFILING
        ++completed;
#endif
        return true;
    };
    std::erase_if(this->retired, collect);
#ifdef NORTHREND_GPU_PROFILING
    this->completedJobs += completed;
    this->report();
#endif
}

bool VulkanTextureBlend::pending() const noexcept {
    return !this->jobs.empty();
}

bool VulkanTextureBlend::ready(const TextureHandle handle) const noexcept {
    const auto matches = [handle](const Job& job) { return job.handle == handle; };

    if (std::ranges::any_of(this->jobs, matches)) {
        return false;
    }

    return std::ranges::none_of(this->retired, [handle](const auto& entry) { return entry.second.handle == handle; });
}

#ifdef NORTHREND_GPU_PROFILING
void VulkanTextureBlend::report() {
    static Interval blendInterval;

    if (!blendInterval.elapsed()) {
        return;
    }

    std::cout << "Preblend: waiting=" << this->jobs.size() << " in_flight=" << this->retired.size()
              << " submitted=" << this->submittedJobs << " completed=" << this->completedJobs
              << " cancelled=" << this->cancelledJobs << '\n';
    this->submittedJobs = 0;
    this->completedJobs = 0;
    this->cancelledJobs = 0;
}
#endif

void VulkanTextureBlend::create_layout() {
    const std::array<VkDescriptorSetLayoutBinding, 4> bindings {
        VkDescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MaximumTextures,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        VkDescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        VkDescriptorSetLayoutBinding {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        VkDescriptorSetLayoutBinding {
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        }
    };
    const VkDescriptorSetLayoutCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    const VkResult result = vkCreateDescriptorSetLayout(this->device.handle(), &info, nullptr, &this->descriptorLayout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan texture blend descriptor layout: " + std::to_string(result));
    }
}

void VulkanTextureBlend::create_descriptors() {
    const std::array<VkDescriptorPoolSize, 3> sizes {
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = MaximumJobs * (MaximumTextures + 1)
        },
        VkDescriptorPoolSize {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = MaximumJobs},
        VkDescriptorPoolSize {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = MaximumJobs}
    };
    const VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = MaximumJobs,
        .poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
        .pPoolSizes = sizes.data()
    };
    VkResult result = vkCreateDescriptorPool(this->device.handle(), &poolInfo, nullptr, &this->descriptorPool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan texture blend descriptor pool: " + std::to_string(result));
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
    result = vkCreateSampler(this->device.handle(), &samplerInfo, nullptr, &this->textureSampler);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan texture blend sampler: " + std::to_string(result));
    }

    VkSamplerCreateInfo alphaInfo = samplerInfo;
    alphaInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    alphaInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    alphaInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    result = vkCreateSampler(this->device.handle(), &alphaInfo, nullptr, &this->alphaSampler);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan texture blend alpha sampler: " + std::to_string(result));
    }
}

void VulkanTextureBlend::create_pipeline() {
    const VkPushConstantRange range {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(std::uint32_t) * 3 + sizeof(float)
    };
    const VkPipelineLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &this->descriptorLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range
    };
    VkResult result = vkCreatePipelineLayout(this->device.handle(), &layoutInfo, nullptr, &this->pipelineLayout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan texture blend pipeline layout: " + std::to_string(result));
    }

    const VkShaderModuleCreateInfo moduleInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = VulkanShader::TextureBlend.size(),
        .pCode = reinterpret_cast<const std::uint32_t*>(VulkanShader::TextureBlend.data())
    };
    VkShaderModule module = VK_NULL_HANDLE;
    result = vkCreateShaderModule(this->device.handle(), &moduleInfo, nullptr, &module);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan texture blend shader: " + std::to_string(result));
    }

    const VkPipelineShaderStageCreateInfo stage {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = module,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };
    const VkComputePipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = stage,
        .layout = this->pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    result =
        vkCreateComputePipelines(this->device.handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline);
    vkDestroyShaderModule(this->device.handle(), module, nullptr);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan texture blend pipeline: " + std::to_string(result));
    }
}

void VulkanTextureBlend::create_commands(const std::size_t batchCount) {
    if (batchCount == 0 || batchCount > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Vulkan texture blend batch count is invalid");
    }

    const VkCommandPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = this->device.graphics_family()
    };
    VkResult result = vkCreateCommandPool(this->device.handle(), &poolInfo, nullptr, &this->commandPool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan texture blend command pool: " + std::to_string(result));
    }

    this->commandBuffers.resize(batchCount);
    const VkCommandBufferAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = this->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<std::uint32_t>(this->commandBuffers.size())
    };
    result = vkAllocateCommandBuffers(this->device.handle(), &allocateInfo, this->commandBuffers.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan texture blend command buffers: " + std::to_string(result));
    }
}

void VulkanTextureBlend::write_descriptor(
    Job& job, const std::span<const TextureHandle> sources, const TextureHandle alpha, const VkImageView output) {
    const VkDescriptorSetAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = this->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &this->descriptorLayout
    };
    VkResult result = vkAllocateDescriptorSets(this->device.handle(), &allocateInfo, &job.descriptor);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a Vulkan texture blend descriptor set: " + std::to_string(result));
    }

    std::array<VkDescriptorImageInfo, MaximumTextures> sourceImages;
    const VulkanTexture* fallback = this->textures.find(sources.front());

    if (fallback == nullptr) {
        throw std::runtime_error("Vulkan texture blend references a missing source texture");
    }

    sourceImages.fill(VkDescriptorImageInfo {
        .sampler = this->textureSampler,
        .imageView = fallback->view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    });

    for (std::size_t index = 0; index < sources.size(); ++index) {
        const VulkanTexture* source = this->textures.find(sources[index]);

        if (source == nullptr) {
            throw std::runtime_error("Vulkan texture blend references a missing source texture");
        }

        sourceImages[index].imageView = source->view();
    }

    const VulkanTexture* alphaTexture = this->textures.find(alpha);

    if (alphaTexture == nullptr) {
        throw std::runtime_error("Vulkan texture blend references a missing alpha texture");
    }

    const VkDescriptorImageInfo alphaImage {
        .sampler = this->alphaSampler,
        .imageView = alphaTexture->view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    const VkDescriptorImageInfo outputImage {
        .sampler = VK_NULL_HANDLE, .imageView = output, .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    const VkDescriptorBufferInfo cells {.buffer = job.cells->handle(), .offset = 0, .range = job.cells->size()};
    const std::array<VkWriteDescriptorSet, 4> writes {
        VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = job.descriptor,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = MaximumTextures,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = sourceImages.data(),
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = job.descriptor,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &alphaImage,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = job.descriptor,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &outputImage,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = job.descriptor,
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &cells,
            .pTexelBufferView = nullptr
        }
    };
    vkUpdateDescriptorSets(this->device.handle(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void VulkanTextureBlend::record(const VkCommandBuffer commandBuffer, const Job& job) const {
    const VkImageSubresourceRange firstMip {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = job.layers
    };
    const VkImageMemoryBarrier computeBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = job.output,
        .subresourceRange = firstMip
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
        nullptr, 0, nullptr, 1, &computeBarrier);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline);
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipelineLayout, 0, 1, &job.descriptor, 0, nullptr);
    const std::array<std::uint32_t, 4> settings {
        job.columns, job.rows, job.cellSize, std::bit_cast<std::uint32_t>(job.textureScale)
    };
    vkCmdPushConstants(
        commandBuffer, this->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(settings), settings.data());
    vkCmdDispatch(commandBuffer, (job.width + 7) / 8, (job.height + 7) / 8, job.layers);

    const VkImageMemoryBarrier sourceBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = job.output,
        .subresourceRange = firstMip
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
        nullptr, 0, nullptr, 1, &sourceBarrier);

    std::int32_t sourceWidth = static_cast<std::int32_t>(job.width);
    std::int32_t sourceHeight = static_cast<std::int32_t>(job.height);

    for (std::uint32_t mip = 1; mip < job.mipCount; ++mip) {
        const std::int32_t destinationWidth = std::max(sourceWidth / 2, 1);
        const std::int32_t destinationHeight = std::max(sourceHeight / 2, 1);
        const VkImageSubresourceRange destinationMip {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = mip,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = job.layers
        };
        const VkImageMemoryBarrier destinationBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = job.output,
            .subresourceRange = destinationMip
        };
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &destinationBarrier);
        const VkImageBlit blit {
            .srcSubresource =
                VkImageSubresourceLayers {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = mip - 1,
                    .baseArrayLayer = 0,
                    .layerCount = job.layers
                },
            .srcOffsets =
                {VkOffset3D {.x = 0, .y = 0, .z = 0}, VkOffset3D {.x = sourceWidth, .y = sourceHeight, .z = 1}},
            .dstSubresource =
                VkImageSubresourceLayers {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = mip,
                    .baseArrayLayer = 0,
                    .layerCount = job.layers
                },
            .dstOffsets = {
                VkOffset3D {.x = 0, .y = 0, .z = 0}, VkOffset3D {.x = destinationWidth, .y = destinationHeight, .z = 1}
            }
        };
        vkCmdBlitImage(commandBuffer, job.output, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, job.output,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
        const VkImageMemoryBarrier nextSourceBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = job.output,
            .subresourceRange = destinationMip
        };
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &nextSourceBarrier);
        sourceWidth = destinationWidth;
        sourceHeight = destinationHeight;
    }

    const VkImageSubresourceRange allMips {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = job.mipCount,
        .baseArrayLayer = 0,
        .layerCount = job.layers
    };
    const VkImageMemoryBarrier sampleBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = job.output,
        .subresourceRange = allMips
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
        nullptr, 0, nullptr, 1, &sampleBarrier);
}

void VulkanTextureBlend::destroy() noexcept {
    this->jobs.clear();
    this->retired.clear();
    this->commandBuffers.clear();

    if (this->commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(this->device.handle(), this->commandPool, nullptr);
        this->commandPool = VK_NULL_HANDLE;
    }

    if (this->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(this->device.handle(), this->pipeline, nullptr);
        this->pipeline = VK_NULL_HANDLE;
    }

    if (this->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(this->device.handle(), this->pipelineLayout, nullptr);
        this->pipelineLayout = VK_NULL_HANDLE;
    }

    if (this->alphaSampler != VK_NULL_HANDLE) {
        vkDestroySampler(this->device.handle(), this->alphaSampler, nullptr);
        this->alphaSampler = VK_NULL_HANDLE;
    }

    if (this->textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(this->device.handle(), this->textureSampler, nullptr);
        this->textureSampler = VK_NULL_HANDLE;
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
