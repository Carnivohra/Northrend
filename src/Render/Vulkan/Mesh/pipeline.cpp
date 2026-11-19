#include "Render/Vulkan/Mesh/pipeline.hpp"

#include "Render/Vulkan/Shader/Mesh/fragment.hpp"
#include "Render/Vulkan/Shader/Mesh/terrain.hpp"
#include "Render/Vulkan/Shader/Mesh/vertex.hpp"
#include "Render/Vulkan/Shader/Mesh/world.hpp"
#include "Render/Mesh/vertex.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Material/store.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace Northrend {

VulkanMeshPipeline::VulkanMeshPipeline(VulkanDevice& device, const VulkanMaterialStore& materials,
    VkRenderPass renderPass, VkExtent2D extent, std::size_t imageCount) :
    device(device), materials(materials), camera(device, imageCount), light(device, imageCount) {
    try {
        this->create_layout();
        this->worldPipeline = this->create_graphics_pipeline(
            renderPass, extent, VulkanShader::MeshWorldVertex, VulkanShader::MeshTerrainFragment, false);

        const std::size_t pipelineCount = this->dynamic_culling() ? BlendModeCount : TransformedPipelineCount;

        for (std::size_t pipeline = 0; pipeline < pipelineCount; ++pipeline) {
            const std::uint32_t blendMode = static_cast<std::uint32_t>(pipeline % BlendModeCount);
            const bool doubleSided = pipeline >= BlendModeCount;
            this->transformedPipelines[pipeline] = this->create_graphics_pipeline(
                renderPass, extent, VulkanShader::MeshVertex, VulkanShader::MeshFragment, true, blendMode, doubleSided);
        }
    } catch (...) {
        this->destroy();
        throw;
    }
}

void VulkanMeshPipeline::update_light(const std::size_t imageIndex, const DirectionalLight& light) {
    this->light.update(imageIndex, light);
}

VulkanMeshPipeline::~VulkanMeshPipeline() {
    this->destroy();
}

void VulkanMeshPipeline::update_camera(
    std::size_t imageIndex, const Camera& camera, const Matrix4& view, std::uint64_t revision, VkExtent2D extent,
    const VkSurfaceTransformFlagBitsKHR surfaceTransform) {
    this->camera.update(imageIndex, camera, view, revision, extent, surfaceTransform);
}

void VulkanMeshPipeline::bind_world(const VkCommandBuffer commandBuffer, const std::size_t imageIndex) const noexcept {
    this->bind(commandBuffer, imageIndex, this->worldPipeline);
}

void VulkanMeshPipeline::bind_transformed(
    const VkCommandBuffer commandBuffer, const std::size_t imageIndex, const std::uint32_t blendMode,
    const bool doubleSided) const noexcept {
    const std::size_t index = std::min<std::size_t>(blendMode, BlendModeCount - 1) +
        (!this->dynamic_culling() && doubleSided ? BlendModeCount : 0);
    this->bind(commandBuffer, imageIndex, this->transformedPipelines[index]);

    if (this->dynamic_culling()) {
        this->set_double_sided(commandBuffer, doubleSided);
    }
}

void VulkanMeshPipeline::set_double_sided(
    const VkCommandBuffer commandBuffer, const bool doubleSided) const noexcept {
    vkCmdSetCullMode(commandBuffer, doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);
}

bool VulkanMeshPipeline::dynamic_culling() const noexcept {
    return this->device.extended_dynamic_state();
}

VkPipelineLayout VulkanMeshPipeline::pipeline_layout() const noexcept {
    return this->layout;
}

void VulkanMeshPipeline::bind(const VkCommandBuffer commandBuffer, const std::size_t imageIndex,
    const VkPipeline pipeline) const noexcept {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    this->camera.bind(commandBuffer, this->layout, imageIndex);
    this->light.bind(commandBuffer, this->layout, imageIndex);
}

void VulkanMeshPipeline::create_layout() {
    const VkDescriptorSetLayout descriptorLayouts[] {
        this->camera.descriptor_layout(), this->light.descriptor_layout(), this->materials.descriptor_layout()
    };
    const VkPushConstantRange detailRange {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(float) * 2 + sizeof(std::uint32_t) * 4
    };
    const VkPipelineLayoutCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 3,
        .pSetLayouts = descriptorLayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &detailRange
    };
    const VkResult result = vkCreatePipelineLayout(this->device.handle(), &createInfo, nullptr, &this->layout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan pipeline layout: " + std::to_string(result));
    }

}

VkPipeline VulkanMeshPipeline::create_graphics_pipeline(const VkRenderPass renderPass, const VkExtent2D extent,
    const std::span<const unsigned char> vertexCode, const std::span<const unsigned char> fragmentCode,
    const bool transformed, const std::uint32_t blendMode, const bool doubleSided) const {
    const VkShaderModule vertexShader = this->create_shader_module(vertexCode);
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

    try {
        fragmentShader = this->create_shader_module(fragmentCode);
    } catch (...) {
        vkDestroyShaderModule(this->device.handle(), vertexShader, nullptr);
        throw;
    }

    const VkPipelineShaderStageCreateInfo shaderStages[] {
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
            .pSpecializationInfo = nullptr
        }
    };
    const std::array<VkVertexInputBindingDescription, 2> transformedBindings {
        VkVertexInputBindingDescription {
            .binding = 0, .stride = sizeof(MeshVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        VkVertexInputBindingDescription {
            .binding = 1, .stride = sizeof(Matrix4), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
        }
    };
    const VkVertexInputBindingDescription worldBinding {
        .binding = 0, .stride = sizeof(MeshVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    const std::array<VkVertexInputAttributeDescription, 7> transformedAttributes {
        VkVertexInputAttributeDescription {
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0
        },
        VkVertexInputAttributeDescription {
            .location = 1, .binding = 0, .format = VK_FORMAT_R8G8B8A8_SNORM, .offset = offsetof(MeshVertex, normal)
        },
        VkVertexInputAttributeDescription {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MeshVertex, textureCoordinates)
        },
        VkVertexInputAttributeDescription {
            .location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = sizeof(float) * 0
        },
        VkVertexInputAttributeDescription {
            .location = 4, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = sizeof(float) * 4
        },
        VkVertexInputAttributeDescription {
            .location = 5, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = sizeof(float) * 8
        },
        VkVertexInputAttributeDescription {
            .location = 6, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = sizeof(float) * 12
        }
    };
    const std::array<VkVertexInputAttributeDescription, 5> worldAttributes {
        VkVertexInputAttributeDescription {
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0
        },
        VkVertexInputAttributeDescription {
            .location = 1, .binding = 0, .format = VK_FORMAT_R8G8B8A8_SNORM, .offset = offsetof(MeshVertex, normal)
        },
        VkVertexInputAttributeDescription {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(MeshVertex, textureCoordinates)
        },
        VkVertexInputAttributeDescription {
            .location = 3, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UINT, .offset = offsetof(MeshVertex, textures)
        },
        VkVertexInputAttributeDescription {
            .location = 4, .binding = 0, .format = VK_FORMAT_R32_UINT, .offset = offsetof(MeshVertex, material)
        }
    };
    const VkPipelineVertexInputStateCreateInfo vertexInput {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = transformed ? static_cast<std::uint32_t>(transformedBindings.size()) : 1,
        .pVertexBindingDescriptions = transformed ? transformedBindings.data() : &worldBinding,
        .vertexAttributeDescriptionCount = transformed ? static_cast<std::uint32_t>(transformedAttributes.size())
                                                       : static_cast<std::uint32_t>(worldAttributes.size()),
        .pVertexAttributeDescriptions = transformed ? transformedAttributes.data() : worldAttributes.data()
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    const VkViewport viewport {
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0F,
        .maxDepth = 1.0F
    };
    const VkRect2D scissor {.offset = VkOffset2D {.x = 0, .y = 0}, .extent = extent};
    const VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    const VkPipelineRasterizationStateCreateInfo rasterization {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = transformed && doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0F,
        .depthBiasClamp = 0.0F,
        .depthBiasSlopeFactor = 0.0F,
        .lineWidth = 1.0F
    };
    const VkDynamicState dynamicStates[] {VK_DYNAMIC_STATE_CULL_MODE};
    const VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = transformed && this->dynamic_culling() ? 1U : 0U,
        .pDynamicStates = dynamicStates
    };
    const VkPipelineMultisampleStateCreateInfo multisampling {
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
    const VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = blendMode < 2 ? VK_TRUE : VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_GREATER,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = VkStencilOpState {},
        .back = VkStencilOpState {},
        .minDepthBounds = 0.0F,
        .maxDepthBounds = 1.0F
    };
    static constexpr std::array<VkBlendFactor, 12> SourceBlend {
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_DST_COLOR,
        VK_BLEND_FACTOR_DST_COLOR,
        VK_BLEND_FACTOR_DST_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_CONSTANT_ALPHA
    };
    static constexpr std::array<VkBlendFactor, 12> DestinationBlend {
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_SRC_COLOR,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA
    };
    const std::size_t blendIndex = std::min<std::size_t>(blendMode, SourceBlend.size() - 1);
    const VkPipelineColorBlendAttachmentState colorAttachment {
        .blendEnable = blendMode >= 2 ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = SourceBlend[blendIndex],
        .dstColorBlendFactor = DestinationBlend[blendIndex],
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    const VkPipelineColorBlendStateCreateInfo colorBlending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .blendConstants = {0.0F, 0.0F, 0.0F, 0.0F}
    };
    const VkGraphicsPipelineCreateInfo createInfo {
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
        .pDynamicState = transformed && this->dynamic_culling() ? &dynamicState : nullptr,
        .layout = this->layout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result =
        vkCreateGraphicsPipelines(this->device.handle(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);

    vkDestroyShaderModule(this->device.handle(), fragmentShader, nullptr);
    vkDestroyShaderModule(this->device.handle(), vertexShader, nullptr);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan graphics pipeline: " + std::to_string(result));
    }

    return pipeline;
}

void VulkanMeshPipeline::destroy() noexcept {
    for (VkPipeline& pipeline : this->transformedPipelines) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(this->device.handle(), pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
    }

    if (this->worldPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(this->device.handle(), this->worldPipeline, nullptr);
        this->worldPipeline = VK_NULL_HANDLE;
    }

    if (this->layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(this->device.handle(), this->layout, nullptr);
        this->layout = VK_NULL_HANDLE;
    }
}

VkShaderModule VulkanMeshPipeline::create_shader_module(std::span<const unsigned char> code) const {
    if (code.empty() || code.size() % sizeof(std::uint32_t) != 0) {
        throw std::runtime_error("Vulkan shader bytecode is invalid");
    }

    const VkShaderModuleCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const std::uint32_t*>(code.data())
    };
    VkShaderModule module = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(this->device.handle(), &createInfo, nullptr, &module);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a Vulkan shader module: " + std::to_string(result));
    }

    return module;
}

}
