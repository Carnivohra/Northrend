#include "Render/Vulkan/pipeline.hpp"

#include "Render/Vulkan/Shader/triangle_fragment.hpp"
#include "Render/Vulkan/Shader/triangle_vertex.hpp"
#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/triangle.hpp"
#include "Render/camera.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend::VulkanPipelineDetail {

Matrix4 multiply(const Matrix4& left, const Matrix4& right) noexcept {
    Matrix4 result;

    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            float value = 0.0F;

            for (std::size_t index = 0; index < 4; ++index) {
                value += left.values[index * 4 + row] *
                    right.values[column * 4 + index];
            }

            result.values[column * 4 + row] = value;
        }
    }

    return result;
}

VkShaderModule create_shader_module(
    VkDevice device,
    std::span<const unsigned char> code) {
    if (code.empty() || code.size() % sizeof(std::uint32_t) != 0) {
        throw std::runtime_error("Vulkan shader bytecode is invalid");
    }

    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const std::uint32_t*>(code.data())
    };
    VkShaderModule module = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(
        device,
        &createInfo,
        nullptr,
        &module);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create a Vulkan shader module: " +
            std::to_string(result));
    }

    return module;
}

}

namespace Northrend {

VulkanPipeline::VulkanPipeline(
    VulkanDevice& device,
    VkRenderPass renderPass,
    VkExtent2D extent,
    std::size_t imageCount)
    : device(device) {
    try {
        this->create_descriptor_layout();
        this->create_camera_buffers(imageCount);
        this->create_descriptors();
        this->create_layout();
        this->create_graphics_pipeline(renderPass, extent);
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanPipeline::~VulkanPipeline() {
    this->destroy();
}

void VulkanPipeline::update_camera(
    std::size_t imageIndex,
    const RenderCamera& camera,
    std::uint64_t revision,
    VkExtent2D extent) {
    const bool extentChanged =
        extent.width != this->cameraExtent.width ||
        extent.height != this->cameraExtent.height;

    if (!this->cameraInitialized ||
        revision != this->cameraSourceRevision ||
        extentChanged) {
        Matrix4 projection = projection_matrix(
            camera,
            static_cast<float>(extent.width) / static_cast<float>(extent.height));

        // Vulkan's framebuffer coordinates use the opposite vertical direction.
        projection.values[5] *= -1.0F;

        this->cameraMatrix = VulkanPipelineDetail::multiply(
            projection,
            camera.view);
        this->cameraExtent = extent;
        this->cameraSourceRevision = revision;
        ++this->cameraUploadRevision;
        this->cameraInitialized = true;
    }

    CameraBuffer& cameraBuffer = this->cameraBuffers[imageIndex];

    if (cameraBuffer.uploadRevision == this->cameraUploadRevision) {
        return;
    }

    std::memcpy(
        cameraBuffer.mapped,
        this->cameraMatrix.values.data(),
        sizeof(this->cameraMatrix));
    cameraBuffer.uploadRevision = this->cameraUploadRevision;
}

void VulkanPipeline::bind(
    VkCommandBuffer commandBuffer,
    std::size_t imageIndex) const noexcept {
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        this->pipeline);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        this->layout,
        0,
        1,
        &this->descriptorSets[imageIndex],
        0,
        nullptr);
}

void VulkanPipeline::push_transform(
    VkCommandBuffer commandBuffer,
    const Matrix4& transform) const noexcept {
    vkCmdPushConstants(
        commandBuffer,
        this->layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(transform),
        &transform);
}

void VulkanPipeline::create_descriptor_layout() {
    const VkDescriptorSetLayoutBinding cameraBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    const VkDescriptorSetLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &cameraBinding
    };
    const VkResult result = vkCreateDescriptorSetLayout(
        this->device.handle(),
        &createInfo,
        nullptr,
        &this->descriptorLayout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan camera descriptor layout: " +
            std::to_string(result));
    }
}

void VulkanPipeline::create_camera_buffers(std::size_t imageCount) {
    this->cameraBuffers.resize(imageCount);

    for (CameraBuffer& cameraBuffer : this->cameraBuffers) {
        cameraBuffer.buffer = std::make_unique<VulkanBuffer>(
            this->device,
            sizeof(Matrix4),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        cameraBuffer.mapped = cameraBuffer.buffer->map();
    }
}

void VulkanPipeline::create_descriptors() {
    const std::uint32_t descriptorCount =
        static_cast<std::uint32_t>(this->cameraBuffers.size());
    const VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = descriptorCount
    };
    const VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = descriptorCount,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    VkResult result = vkCreateDescriptorPool(
        this->device.handle(),
        &poolInfo,
        nullptr,
        &this->descriptorPool);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan camera descriptor pool: " +
            std::to_string(result));
    }

    const std::vector<VkDescriptorSetLayout> layouts(
        this->cameraBuffers.size(),
        this->descriptorLayout);
    this->descriptorSets.resize(this->cameraBuffers.size());

    const VkDescriptorSetAllocateInfo allocationInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = this->descriptorPool,
        .descriptorSetCount = descriptorCount,
        .pSetLayouts = layouts.data()
    };
    result = vkAllocateDescriptorSets(
        this->device.handle(),
        &allocationInfo,
        this->descriptorSets.data());

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to allocate Vulkan camera descriptors: " +
            std::to_string(result));
    }

    for (std::size_t index = 0; index < this->cameraBuffers.size(); ++index) {
        const VkDescriptorBufferInfo bufferInfo{
            .buffer = this->cameraBuffers[index].buffer->handle(),
            .offset = 0,
            .range = sizeof(Matrix4)
        };
        const VkWriteDescriptorSet write{
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
        vkUpdateDescriptorSets(
            this->device.handle(),
            1,
            &write,
            0,
            nullptr);
    }
}

void VulkanPipeline::create_layout() {
    const VkPushConstantRange instanceRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(Matrix4)
    };
    const VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &this->descriptorLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &instanceRange
    };
    const VkResult result = vkCreatePipelineLayout(
        this->device.handle(),
        &createInfo,
        nullptr,
        &this->layout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan pipeline layout: " +
            std::to_string(result));
    }
}

void VulkanPipeline::create_graphics_pipeline(
    VkRenderPass renderPass,
    VkExtent2D extent) {
    const VkShaderModule vertexShader = VulkanPipelineDetail::create_shader_module(
        this->device.handle(),
        VulkanShader::TriangleVertex);
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

    try {
        fragmentShader = VulkanPipelineDetail::create_shader_module(
            this->device.handle(),
            VulkanShader::TriangleFragment);
    } catch (...) {
        vkDestroyShaderModule(this->device.handle(), vertexShader, nullptr);
        throw;
    }

    const VkPipelineShaderStageCreateInfo shaderStages[]{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
            .pSpecializationInfo = nullptr
        }
    };
    const VkVertexInputBindingDescription vertexBinding{
        .binding = 0,
        .stride = sizeof(VulkanTriangle::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    const std::array<VkVertexInputAttributeDescription, 2> vertexAttributes{
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VulkanTriangle::Vertex, position)
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VulkanTriangle::Vertex, color)
        }
    };
    const VkPipelineVertexInputStateCreateInfo vertexInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBinding,
        .vertexAttributeDescriptionCount =
            static_cast<std::uint32_t>(vertexAttributes.size()),
        .pVertexAttributeDescriptions = vertexAttributes.data()
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    const VkViewport viewport{
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0F,
        .maxDepth = 1.0F
    };
    const VkRect2D scissor{
        .offset = VkOffset2D{ .x = 0, .y = 0 },
        .extent = extent
    };
    const VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    const VkPipelineRasterizationStateCreateInfo rasterization{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0F,
        .depthBiasClamp = 0.0F,
        .depthBiasSlopeFactor = 0.0F,
        .lineWidth = 1.0F
    };
    const VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };
    const VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = VkStencilOpState{},
        .back = VkStencilOpState{},
        .minDepthBounds = 0.0F,
        .maxDepthBounds = 1.0F
    };
    const VkPipelineColorBlendAttachmentState colorAttachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
    };
    const VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .blendConstants = { 0.0F, 0.0F, 0.0F, 0.0F }
    };
    const VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = nullptr,
        .layout = this->layout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    const VkResult result = vkCreateGraphicsPipelines(
        this->device.handle(),
        VK_NULL_HANDLE,
        1,
        &createInfo,
        nullptr,
        &this->pipeline);

    vkDestroyShaderModule(this->device.handle(), fragmentShader, nullptr);
    vkDestroyShaderModule(this->device.handle(), vertexShader, nullptr);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create the Vulkan graphics pipeline: " +
            std::to_string(result));
    }
}

void VulkanPipeline::destroy() noexcept {
    if (this->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(this->device.handle(), this->pipeline, nullptr);
        this->pipeline = VK_NULL_HANDLE;
    }

    if (this->layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(this->device.handle(), this->layout, nullptr);
        this->layout = VK_NULL_HANDLE;
    }

    if (this->descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(this->device.handle(), this->descriptorPool, nullptr);
        this->descriptorPool = VK_NULL_HANDLE;
    }
    this->descriptorSets.clear();

    this->cameraBuffers.clear();

    if (this->descriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            this->device.handle(),
            this->descriptorLayout,
            nullptr);
        this->descriptorLayout = VK_NULL_HANDLE;
    }
}

}
