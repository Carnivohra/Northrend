#pragma once

#include "Render/Material/material.hpp"
#include "Render/Mesh/mesh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Northrend {

struct RenderSections final {
    static constexpr std::size_t Count = 16;

    MeshHandle mesh;
    MaterialHandle material;
    std::array<std::uint8_t, Count> levels {};
    std::uint16_t visibility = 0;
};

static_assert(std::is_trivially_copyable_v<RenderSections>);
static_assert(std::is_standard_layout_v<RenderSections>);

}
