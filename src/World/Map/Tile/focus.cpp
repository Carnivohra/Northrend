#include "World/Map/Tile/focus.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Northrend {

TileFocus::TileFocus(const std::size_t x, const std::size_t y, const std::size_t radius) :
    tileX(x), tileY(y), tileRadius(std::min(radius, Size - 1)), focused(true) {
    if (x >= Size || y >= Size) {
        throw std::out_of_range("Tile focus is outside the map");
    }
}

TileFocus TileFocus::from_world(const float x, const float y, const std::size_t radius) noexcept {
    static constexpr float TileSize = 533.3333333F;
    static constexpr float Center = static_cast<float>(Size / 2);
    const float tileX = std::floor(Center - y / TileSize);
    const float tileY = std::floor(Center - x / TileSize);

    if (!std::isfinite(tileX) || !std::isfinite(tileY) || tileX < 0.0F || tileY < 0.0F ||
        tileX >= static_cast<float>(Size) || tileY >= static_cast<float>(Size)) {
        return {};
    }

    return TileFocus(static_cast<std::size_t>(tileX), static_cast<std::size_t>(tileY), radius);
}

bool TileFocus::active() const noexcept {
    return this->focused;
}

std::size_t TileFocus::x() const noexcept {
    return this->tileX;
}

std::size_t TileFocus::y() const noexcept {
    return this->tileY;
}

std::size_t TileFocus::radius() const noexcept {
    return this->tileRadius;
}

}
