#include "Presentation/map.hpp"

#include "World/Map/Tile/change.hpp"
#include "World/Map/Tile/tile.hpp"

#include <utility>

namespace Northrend {

void PresentationMap::apply(
    TileChange change, MeshHandleAllocator& allocator, Renderer& renderer, RenderScene& scene) {
    if (change.kind() == TileChange::Kind::reset) {
        this->terrain.clear(renderer, scene);
        this->tiles.fill(nullptr);
        this->currentGeneration = change.generation();
        return;
    }

    if (change.generation() != this->currentGeneration || change.x() >= TileGrid::Size ||
        change.y() >= TileGrid::Size) {
        return;
    }

    const std::size_t x = change.x();
    const std::size_t y = change.y();
    const std::size_t tile = index(x, y);

    if (change.kind() == TileChange::Kind::unloaded) {
        this->terrain.unload(x, y, renderer, scene);
        this->tiles[tile].reset();
        return;
    }

    std::shared_ptr<const MapTile> mapTile = std::move(change).tile();

    if (!mapTile) {
        return;
    }

    this->terrain.load(x, y, *mapTile, allocator, renderer, scene);
    this->tiles[tile] = std::move(mapTile);
}

void PresentationMap::clear(Renderer& renderer, RenderScene& scene) {
    this->terrain.clear(renderer, scene);
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
