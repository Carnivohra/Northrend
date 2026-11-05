#include "World/Map/layout.hpp"

namespace Northrend {

MapLayout::MapLayout(const std::span<const std::byte> data) : mapDefinition(data) {}

bool MapLayout::has_tile(const std::size_t x, const std::size_t y) const noexcept {
    return this->mapDefinition.has_tile(x, y);
}

bool MapLayout::high_resolution_alpha() const noexcept {
    static constexpr std::uint32_t HighResolutionAlpha = 0x4;
    return (this->mapDefinition.flags() & HighResolutionAlpha) != 0;
}

}
