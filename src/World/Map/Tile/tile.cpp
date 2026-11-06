#include "World/Map/Tile/tile.hpp"

namespace Northrend {

MapTile::MapTile(const Adt& adt) : terrainGeometry(adt), terrainMaterial(adt) {}

const TerrainGeometry& MapTile::terrain() const noexcept {
    return this->terrainGeometry;
}

const TerrainMaterial& MapTile::material() const noexcept {
    return this->terrainMaterial;
}

}
