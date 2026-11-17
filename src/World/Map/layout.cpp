#include "World/Map/layout.hpp"

namespace Northrend {

MapLayout::MapLayout(const std::span<const std::byte> data) : mapDefinition(data) {
    static constexpr std::size_t Size = 64;
    static constexpr double TileSize = 533.3333333;
    static constexpr double GridCenter = static_cast<double>(Size) / 2.0;
    double x = 0.0;
    double y = 0.0;
    std::size_t count = 0;

    for (std::size_t tileY = 0; tileY < Size; ++tileY) {
        for (std::size_t tileX = 0; tileX < Size; ++tileX) {
            if (!this->mapDefinition.has_tile(tileX, tileY)) {
                continue;
            }

            x += (GridCenter - static_cast<double>(tileY) - 0.5) * TileSize;
            y += (GridCenter - static_cast<double>(tileX) - 0.5) * TileSize;
            ++count;
        }
    }

    if (count != 0) {
        this->mapCenter = Vector3 {
            .x = static_cast<float>(x / static_cast<double>(count)),
            .y = static_cast<float>(y / static_cast<double>(count)),
            .z = 0.0F
        };
    }
}

bool MapLayout::has_tile(const std::size_t x, const std::size_t y) const noexcept {
    return this->mapDefinition.has_tile(x, y);
}

bool MapLayout::high_resolution_alpha() const noexcept {
    static constexpr std::uint32_t HighResolutionAlpha = 0x4;
    return (this->mapDefinition.flags() & HighResolutionAlpha) != 0;
}

const std::optional<Vector3>& MapLayout::center() const noexcept {
    return this->mapCenter;
}

}
