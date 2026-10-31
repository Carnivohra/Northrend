#include "Render/Vulkan/Mesh/store.hpp"

#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/Mesh/mesh.hpp"

#include <limits>
#include <stdexcept>

namespace Northrend {

VulkanMeshStore::VulkanMeshStore(VulkanDevice& device) noexcept : device(device), arena(device) {}

VulkanMeshStore::~VulkanMeshStore() = default;

void VulkanMeshStore::store(const MeshHandle handle, const MeshData& data) {
    if (!handle) {
        throw std::invalid_argument("Cannot store a Vulkan mesh without a handle");
    }

    const std::uint64_t rawIndex = handle.id() - 1;

    if (rawIndex >= std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("Mesh handle exceeds the platform index range");
    }

    const std::size_t index = static_cast<std::size_t>(rawIndex);

    if (this->meshes.size() <= index) {
        this->meshes.resize(index + 1);
    }

    if (this->meshes[index]) {
        throw std::invalid_argument("A Vulkan mesh already exists for this handle");
    }

    this->meshes[index] = std::make_unique<VulkanMesh>(this->arena, data);
    ++this->currentRevision;
}

void VulkanMeshStore::erase(const MeshHandle handle) noexcept {
    if (!handle) {
        return;
    }

    const std::uint64_t index = handle.id() - 1;

    if (index >= this->meshes.size() || !this->meshes[static_cast<std::size_t>(index)]) {
        return;
    }

    this->device.wait_idle();
    this->meshes[static_cast<std::size_t>(index)].reset();
    ++this->currentRevision;
}

const VulkanMesh* VulkanMeshStore::find(MeshHandle handle) const noexcept {
    if (!handle) {
        return nullptr;
    }

    const std::uint64_t index = handle.id() - 1;

    if (index >= this->meshes.size()) {
        return nullptr;
    }

    return this->meshes[static_cast<std::size_t>(index)].get();
}

VkBuffer VulkanMeshStore::buffer() const noexcept {
    return this->arena.handle();
}

std::uint64_t VulkanMeshStore::revision() const noexcept {
    return this->currentRevision + this->arena.revision();
}

}
