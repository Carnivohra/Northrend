#include "Render/Vulkan/Pipeline/mesh.hpp"

#include "Render/Vulkan/Shader/Mesh/fragment.hpp"
#include "Render/Vulkan/Shader/Mesh/vertex.hpp"
#include "Render/Vulkan/Mesh/vertex.hpp"
#include "Render/Vulkan/device.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanMeshPipeline::VulkanMeshPipeline(
    VulkanDevice& device,
    VkRenderPass renderPass,
    VkExtent2D extent,
    std::size_t imageCount)
    : device(device),
      camera(device, imageCount) {
    try {
        this->create_layout();
        this->create_graphics_pipeline(renderPass, extent);
    } catch (...) {
        this->destroy();
        throw;
    }
}

VulkanMeshPipeline::~VulkanMeshPipeline() {
    this->destroy();
}

void VulkanMeshPipeline::update_camera(
    std::size_t imageIndex,
    const Camera& camera,
    const Matrix4& view,
    std::uint64_t revision,
    VkExtent2D extent) {
    this->camera.update(
        imageIndex,
        camera,
        view,
        revision,
        extent);
}

void VulkanMeshPipeline::bind(
    VkCommandBuffer commandBuffer,
    std::size_t imageIndex) const noexcept {
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        this->pipeline);
    this->camera.bind(
        commandBuffer,
        this->layout,
        imageIndex);
}

void VulkanMeshPipeline::push_transform(
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

void VulkanMeshPipeline::create_layout() {
    const VkDescriptorSetLayout cameraLayout =
        this->camera.descriptor_layout();
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
        .pSetLayouts = &cameraLayout,
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

void VulkanMeshPipeline::create_graphics_pipeline(
    VkRenderPass renderPass,
    VkExtent2D extent) {
    const VkShaderModule vertexShader = this->create_shader_module(
        VulkanShader::MeshVertex);
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

    try {
        fragmentShader = this->create_shader_module(
            VulkanShader::MeshFragment);
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
        .stride = sizeof(VulkanMeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    const std::array<VkVertexInputAttributeDescription, 2> vertexAttributes{
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VulkanMeshVertex, position)
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VulkanMeshVertex, color)
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

void VulkanMeshPipeline::destroy() noexcept {
    if (this->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(
            this->device.handle(),
            this->pipeline,
            nullptr);
        this->pipeline = VK_NULL_HANDLE;
    }

    if (this->layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(
            this->device.handle(),
            this->layout,
            nullptr);
        this->layout = VK_NULL_HANDLE;
    }
}

VkShaderModule VulkanMeshPipeline::create_shader_module(
    std::span<const unsigned char> code) const {
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
        this->device.handle(),
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
