#pragma once

#include "Math/matrix.hpp"
#include "Render/Material/material.hpp"
#include "Render/Mesh/mesh.hpp"

#include <cstdint>
#include <type_traits>

namespace Northrend {

struct RenderInstance final {
    MeshHandle mesh;
    MaterialHandle material;
    Matrix4 transform;
    std::uint8_t level = 0;
    bool visible = true;
    bool worldSpace = false;
};

static_assert(std::is_trivially_copyable_v<RenderInstance>);
static_assert(std::is_standard_layout_v<RenderInstance>);

}
