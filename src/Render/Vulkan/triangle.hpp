#pragma once

#include "Math/vector.hpp"

#include <memory>
#include <type_traits>

namespace Northrend {

class VulkanDevice;
class VulkanMesh;

namespace VulkanTriangle {

struct Vertex final {
    Vector3 position;
    Vector3 color;
};

static_assert(std::is_trivially_copyable_v<Vertex>);
static_assert(std::is_standard_layout_v<Vertex>);

[[nodiscard]] std::unique_ptr<VulkanMesh> create(VulkanDevice& device);

}

}
