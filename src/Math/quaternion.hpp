#pragma once

#include <type_traits>

namespace Northrend {

struct Quaternion final {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

static_assert(sizeof(Quaternion) == sizeof(float) * 4);
static_assert(std::is_trivially_copyable_v<Quaternion>);
static_assert(std::is_standard_layout_v<Quaternion>);

}
