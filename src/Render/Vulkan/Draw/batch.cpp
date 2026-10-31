#include "Render/Vulkan/Draw/batch.hpp"

#include "Render/Vulkan/Mesh/mesh.hpp"
#include "Render/Vulkan/Mesh/store.hpp"
#include "Render/Vulkan/buffer.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/frame.hpp"

#include <limits>
#include <span>
#include <stdexcept>

namespace Northrend {

VulkanDrawBatch::VulkanDrawBatch(VulkanDevice& device) noexcept : device(device) {}

VulkanDrawBatch::~VulkanDrawBatch() = default;

void VulkanDrawBatch::allocate(const std::size_t count) {
    this->storage.clear();
    this->storage.resize(count);
}

void VulkanDrawBatch::release() noexcept {
    this->storage.clear();
}

void VulkanDrawBatch::update(
    const std::size_t imageIndex, const RenderFrame& frame, const VulkanMeshStore& meshes) {
    Storage& target = this->storage.at(imageIndex);
    const std::uint64_t meshRevision = meshes.revision();

    if (target.initialized && target.instanceRevision == frame.instancesRevision &&
        target.meshRevision == meshRevision) {
        return;
    }

    std::vector<Matrix4> transforms;
    transforms.reserve(frame.instances.size());
    target.unsigned16.clear();
    target.unsigned32.clear();
    target.unsigned16.reserve(frame.instances.size());
    target.unsigned32.reserve(frame.instances.size());

    for (const RenderInstance& instance : frame.instances) {
        const VulkanMesh* mesh = meshes.find(instance.mesh);

        if (!mesh) {
            continue;
        }

        if (transforms.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("Vulkan draw batch exceeds the instance index range");
        }

        const std::uint32_t firstInstance = static_cast<std::uint32_t>(transforms.size());
        transforms.push_back(instance.transform);
        auto& commands = mesh->index_type() == VK_INDEX_TYPE_UINT16 ? target.unsigned16 : target.unsigned32;
        commands.push_back(mesh->draw(firstInstance));
    }

    const VkDeviceSize required = transforms.size() * sizeof(Matrix4);

    if (required != 0 && (!target.buffer || target.buffer->size() < required)) {
        const VkDeviceSize byteCapacity = capacity(target.buffer ? target.buffer->size() : 0, required);
        target.buffer = std::make_unique<VulkanBuffer>(this->device, byteCapacity,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        target.staging = std::make_unique<VulkanBuffer>(this->device, byteCapacity, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    if (required != 0) {
        target.staging->write(std::as_bytes(std::span<const Matrix4>(transforms)));
        this->device.copy_buffer(target.staging->handle(), target.buffer->handle(), required);
    }

    target.instanceRevision = frame.instancesRevision;
    target.meshRevision = meshRevision;
    target.initialized = true;
}

void VulkanDrawBatch::draw(const VkCommandBuffer commandBuffer, const std::size_t imageIndex,
    const VulkanMeshStore& meshes) const noexcept {
    const Storage& source = this->storage[imageIndex];

    if (!source.buffer || (source.unsigned16.empty() && source.unsigned32.empty())) {
        return;
    }

    const VkBuffer meshBuffer = meshes.buffer();
    const VkBuffer vertexBuffers[] {meshBuffer, source.buffer->handle()};
    constexpr VkDeviceSize offsets[] {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);

    if (!source.unsigned16.empty()) {
        vkCmdBindIndexBuffer(commandBuffer, meshBuffer, 0, VK_INDEX_TYPE_UINT16);
        draw_group(commandBuffer, source.unsigned16);
    }

    if (!source.unsigned32.empty()) {
        vkCmdBindIndexBuffer(commandBuffer, meshBuffer, 0, VK_INDEX_TYPE_UINT32);
        draw_group(commandBuffer, source.unsigned32);
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

void VulkanDrawBatch::draw_group(const VkCommandBuffer commandBuffer, const std::vector<VulkanDraw>& draws) noexcept {
    for (const VulkanDraw& draw : draws) {
        vkCmdDrawIndexed(
            commandBuffer, draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, draw.firstInstance);
    }
}

}
