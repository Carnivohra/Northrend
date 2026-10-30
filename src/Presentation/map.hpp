#pragma once

#include "World/Map/Tile/grid.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace Northrend {

class MapTile;
class TileChange;

class PresentationMap final {
public:
    void apply(TileChange change) noexcept;
    void clear() noexcept;

    [[nodiscard]] const MapTile* tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

private:
    [[nodiscard]] static constexpr std::size_t index(const std::size_t x, const std::size_t y) noexcept {
        return y * TileGrid::Size + x;
    }

    std::array<std::shared_ptr<const MapTile>, TileGrid::TileCount> tiles;
    std::uint32_t currentGeneration = 0;
};

}
