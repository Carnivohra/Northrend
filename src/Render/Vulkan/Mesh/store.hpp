#pragma once

#include "Render/Mesh/data.hpp"
#include "Render/Mesh/mesh.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

class VulkanDevice;
class VulkanMesh;

class VulkanMeshStore final {
public:
    explicit VulkanMeshStore(VulkanDevice& device) noexcept;
    ~VulkanMeshStore();

    VulkanMeshStore(const VulkanMeshStore&) = delete;
    VulkanMeshStore& operator=(const VulkanMeshStore&) = delete;

    void store(MeshHandle handle, const MeshData& data);
    void erase(MeshHandle handle) noexcept;
    [[nodiscard]] const VulkanMesh* find(MeshHandle handle) const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;

private:
    VulkanDevice& device;
    std::vector<std::unique_ptr<VulkanMesh>> meshes;
    std::uint64_t currentRevision = 0;
};

}
