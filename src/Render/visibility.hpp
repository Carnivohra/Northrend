#pragma once

#include <type_traits>

namespace Northrend {

struct RenderVisibility final {
    bool visible = true;
};

static_assert(std::is_trivially_copyable_v<RenderVisibility>);
static_assert(std::is_standard_layout_v<RenderVisibility>);

}
