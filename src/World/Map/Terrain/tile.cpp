#include "World/Map/Terrain/tile.hpp"

#include "Asset/adt.hpp"

namespace Northrend {

TerrainTile::TerrainTile(const Adt& adt) : terrainGeometry(adt) {}

const TerrainGeometry& TerrainTile::geometry() const noexcept {
    return this->terrainGeometry;
}

}
