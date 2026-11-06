#include "Render/Vulkan/Draw/batch.hpp"

#include "Render/Vulkan/Mesh/mesh.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/Material/store.hpp"
#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/frame.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>

namespace Northrend {

VulkanDrawBatch::VulkanDrawBatch(VulkanDevice& device) noexcept : device(device) {}

VulkanDrawBatch::~VulkanDrawBatch() = default;

void VulkanDrawBatch::allocate(const std::size_t count) {
    this->stores.clear();
    this->stores.resize(count);
}

void VulkanDrawBatch::release() noexcept {
    this->stores.clear();
}

void VulkanDrawBatch::update(const std::size_t imageIndex, const RenderFrame& frame, const VulkanMeshStore& meshes) {
    Store& target = this->stores.at(imageIndex);
    const std::uint64_t meshRevision = meshes.revision();

    if (target.initialized && target.instanceRevision == frame.instancesRevision &&
        target.meshRevision == meshRevision) {
        return;
    }

    std::vector<Matrix4> transforms;
    transforms.reserve(frame.instances.size());
    target.worldUnsigned16.resize(meshes.page_count());
    target.worldUnsigned32.resize(meshes.page_count());
    target.transformedUnsigned16.resize(meshes.page_count());
    target.transformedUnsigned32.resize(meshes.page_count());
    target.worldDraws = false;
    target.transformedDraws = false;

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        target.worldUnsigned16[page].clear();
        target.worldUnsigned32[page].clear();
        target.transformedUnsigned16[page].clear();
        target.transformedUnsigned32[page].clear();
    }

    for (const RenderInstance& instance : frame.instances) {
        const VulkanMesh* mesh = meshes.find(instance.mesh);

        if (!mesh) {
            continue;
        }

        if (instance.worldSpace) {
            auto& pages = mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.worldUnsigned16 : target.worldUnsigned32;
            VulkanDraw draw = mesh->draw(0, instance.level);
            draw.detailLevel = instance.level;
            draw.material = instance.material;
            pages[mesh->page()].push_back(draw);
            target.worldDraws = true;
            continue;
        }

        if (transforms.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("Vulkan draw batch exceeds the instance index range");
        }

        const std::uint32_t firstInstance = static_cast<std::uint32_t>(transforms.size());
        transforms.push_back(instance.transform);
        auto& pages =
            mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.transformedUnsigned16 : target.transformedUnsigned32;
        auto& commands = pages[mesh->page()];
        commands.push_back(mesh->draw(firstInstance, instance.level));
        target.transformedDraws = true;
    }

    const VkDeviceSize required = transforms.size() * sizeof(Matrix4);

    if (required != 0 && (!target.transformBuffer || target.transformBuffer->size() < required)) {
        const VkDeviceSize byteCapacity =
            capacity(target.transformBuffer ? target.transformBuffer->size() : 0, required);
        target.transformBuffer = std::make_unique<VulkanBuffer>(this->device, byteCapacity,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        target.transformStaging =
            std::make_unique<VulkanBuffer>(this->device, byteCapacity, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    if (required != 0) {
        target.transformStaging->write(std::as_bytes(std::span<const Matrix4>(transforms)));
        this->device.copy_buffer(target.transformStaging->handle(), target.transformBuffer->handle(), required);
    }

    target.instanceRevision = frame.instancesRevision;
    target.meshRevision = meshRevision;
    target.initialized = true;
}

bool VulkanDrawBatch::has_world(const std::size_t imageIndex) const noexcept {
    return this->stores[imageIndex].worldDraws;
}

bool VulkanDrawBatch::has_transformed(const std::size_t imageIndex) const noexcept {
    return this->stores[imageIndex].transformedDraws;
}

void VulkanDrawBatch::draw_world(const VkCommandBuffer commandBuffer, const std::size_t imageIndex,
    const VulkanMeshStore& meshes, const VulkanMaterialStore& materials,
    const VkPipelineLayout pipelineLayout) const noexcept {
    const Store& source = this->stores[imageIndex];

    if (!source.worldDraws) {
        return;
    }

    constexpr VkDeviceSize offset = 0;

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        const auto& unsigned16 = source.worldUnsigned16[page];
        const auto& unsigned32 = source.worldUnsigned32[page];

        if (unsigned16.empty() && unsigned32.empty()) {
            continue;
        }

        const VkBuffer meshBuffer = meshes.buffer(page);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshBuffer, &offset);

        if (!unsigned16.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, meshBuffer, 0, VK_INDEX_TYPE_UINT16);
            draw_group(commandBuffer, unsigned16, &materials, pipelineLayout);
        }

        if (!unsigned32.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, meshBuffer, 0, VK_INDEX_TYPE_UINT32);
            draw_group(commandBuffer, unsigned32, &materials, pipelineLayout);
        }
    }
}

void VulkanDrawBatch::draw_transformed(
    const VkCommandBuffer commandBuffer, const std::size_t imageIndex, const VulkanMeshStore& meshes) const noexcept {
    const Store& source = this->stores[imageIndex];

    if (!source.transformedDraws || !source.transformBuffer) {
        return;
    }

    constexpr VkDeviceSize offsets[] {0, 0};

    for (std::size_t page = 0; page < meshes.page_count(); ++page) {
        const auto& unsigned16 = source.transformedUnsigned16[page];
        const auto& unsigned32 = source.transformedUnsigned32[page];

        if (unsigned16.empty() && unsigned32.empty()) {
            continue;
        }

        const VkBuffer vertexBuffers[] {meshes.buffer(page), source.transformBuffer->handle()};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

        if (!unsigned16.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffers[0], 0, VK_INDEX_TYPE_UINT16);
            draw_group(commandBuffer, unsigned16);
        }

        if (!unsigned32.empty()) {
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffers[0], 0, VK_INDEX_TYPE_UINT32);
            draw_group(commandBuffer, unsigned32);
        }
    }
}

VkDeviceSize VulkanDrawBatch::capacity(const VkDeviceSize current, const VkDeviceSize required) {
    VkDeviceSize result = current == 0 ? InitialCapacity : current;

    while (result < required) {
        if (result > std::numeric_limits<VkDeviceSize>::max() / 2) {
            return required;
        }

        result *= 2;
    }

    return result;
}

void VulkanDrawBatch::draw_group(const VkCommandBuffer commandBuffer, const std::vector<VulkanDraw>& draws,
    const VulkanMaterialStore* materials, const VkPipelineLayout pipelineLayout) noexcept {
    static constexpr std::array<float, 3> TextureMipBias {0.0F, 0.75F, 1.5F};
    static constexpr std::array<float, 3> AlphaMipBias {0.0F, 1.0F, 2.0F};

    for (const VulkanDraw& draw : draws) {
        if (materials != nullptr) {
            const std::size_t level = std::min<std::size_t>(draw.detailLevel, TextureMipBias.size() - 1);
            const std::array<float, 2> detail {TextureMipBias[level], AlphaMipBias[level]};
            vkCmdPushConstants(
                commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(detail), detail.data());
            materials->bind(commandBuffer, pipelineLayout, draw.material);
        }

        vkCmdDrawIndexed(commandBuffer, draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, draw.firstInstance);
    }
}

}
