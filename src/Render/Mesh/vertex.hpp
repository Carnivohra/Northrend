#pragma once

#include "Math/vector.hpp"

#include <array>
#include <cstdint>
#include <type_traits>

namespace Northrend {

struct MeshVertex final {
    Vector3 position;
    std::array<std::int8_t, 4> normal {0, 0, 127, 0};
};

static_assert(sizeof(MeshVertex) == sizeof(float) * 4);
static_assert(std::is_trivially_copyable_v<MeshVertex>);
static_assert(std::is_standard_layout_v<MeshVertex>);

}
