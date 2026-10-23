#pragma once

#include <array>
#include <type_traits>

namespace Northrend {

struct Matrix4 final {
    std::array<float, 16> values{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
};

static_assert(sizeof(Matrix4) == sizeof(float) * 16);
static_assert(std::is_trivially_copyable_v<Matrix4>);
static_assert(std::is_standard_layout_v<Matrix4>);

}
