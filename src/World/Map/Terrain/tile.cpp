#include "World/Map/Terrain/tile.hpp"

#include <utility>

namespace Northrend {

TerrainTile::TerrainTile(std::shared_ptr<const AssetData> data) :
    terrainData(std::move(data)), terrainGeometry(this->terrainData) {}

const Adt& TerrainTile::adt() const noexcept {
    return this->terrainData;
}

const TerrainGeometry& TerrainTile::geometry() const noexcept {
    return this->terrainGeometry;
}

}
