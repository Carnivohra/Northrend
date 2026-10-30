#include "World/Map/Tile/tile.hpp"

#include "Asset/adt.hpp"

#include <utility>

namespace Northrend {

MapTile::MapTile(std::shared_ptr<const AssetData> data) : mapTerrain(Adt(std::move(data))) {}

const TerrainTile& MapTile::terrain() const noexcept {
    return this->mapTerrain;
}

}
