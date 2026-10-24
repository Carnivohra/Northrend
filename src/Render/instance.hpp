#pragma once

#include "Math/matrix.hpp"
#include "Render/mesh.hpp"

#include <type_traits>

namespace Northrend {

struct RenderInstance final {
    MeshHandle mesh;
    Matrix4 transform;
};

static_assert(std::is_trivially_copyable_v<RenderInstance>);
static_assert(std::is_standard_layout_v<RenderInstance>);

}
