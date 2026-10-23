#pragma once

#include <type_traits>

namespace Northrend {

struct Vector3 final {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

static_assert(sizeof(Vector3) == sizeof(float) * 3);
static_assert(std::is_trivially_copyable_v<Vector3>);
static_assert(std::is_standard_layout_v<Vector3>);

}
