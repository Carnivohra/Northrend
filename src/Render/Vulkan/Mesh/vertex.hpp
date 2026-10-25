#pragma once

#include "Math/vector.hpp"

#include <type_traits>

namespace Northrend {

struct VulkanMeshVertex final {
    Vector3 position;
    Vector3 color;
};

static_assert(std::is_trivially_copyable_v<VulkanMeshVertex>);
static_assert(std::is_standard_layout_v<VulkanMeshVertex>);

}
