#pragma once

#include "Math/vector.hpp"

#include <array>
#include <cstdint>
#include <type_traits>

namespace Northrend {

struct MeshVertex final {
    Vector3 position;
    std::array<std::int8_t, 4> normal {0, 0, 127, 0};
    std::array<float, 2> textureCoordinates {};
    std::uint32_t texture = 0;
    std::uint32_t padding = 0;
};

static_assert(sizeof(MeshVertex) == sizeof(float) * 8);
static_assert(std::is_trivially_copyable_v<MeshVertex>);
static_assert(std::is_standard_layout_v<MeshVertex>);

}
