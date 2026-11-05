#include "World/Map/Tile/tile.hpp"

#include "Asset/Format/adt.hpp"

#include <utility>

namespace Northrend {

MapTile::MapTile(std::shared_ptr<const AssetData> data) : terrainGeometry(Adt(std::move(data))) {}

const TerrainGeometry& MapTile::terrain() const noexcept {
    return this->terrainGeometry;
}

}
