#pragma once

#include "Math/vector.hpp"
#include "Render/mesh.hpp"

#include <type_traits>

namespace Northrend {

class VulkanMeshStore;

namespace VulkanTriangle {

struct Vertex final {
    Vector3 position;
    Vector3 color;
};

static_assert(std::is_trivially_copyable_v<Vertex>);
static_assert(std::is_standard_layout_v<Vertex>);

[[nodiscard]] MeshHandle create(VulkanMeshStore& meshes);

}

}
