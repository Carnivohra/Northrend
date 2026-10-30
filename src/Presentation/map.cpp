#include "Presentation/map.hpp"

#include "World/Map/Tile/change.hpp"

#include <utility>

namespace Northrend {

void PresentationMap::apply(TileChange change) noexcept {
    if (change.kind() == TileChange::Kind::reset) {
        this->tiles.fill(nullptr);
        this->currentGeneration = change.generation();
        return;
    }

    if (change.generation() != this->currentGeneration || change.x() >= TileGrid::Size ||
        change.y() >= TileGrid::Size) {
        return;
    }

    const std::size_t tile = index(change.x(), change.y());

    if (change.kind() == TileChange::Kind::unloaded) {
        this->tiles[tile].reset();
        return;
    }

    this->tiles[tile] = std::move(change).tile();
}

void PresentationMap::clear() noexcept {
    this->tiles.fill(nullptr);
    this->currentGeneration = 0;
}

const MapTile* PresentationMap::tile(const std::size_t x, const std::size_t y) const noexcept {
    return x < TileGrid::Size && y < TileGrid::Size ? this->tiles[index(x, y)].get() : nullptr;
}

std::uint32_t PresentationMap::generation() const noexcept {
    return this->currentGeneration;
}

}
