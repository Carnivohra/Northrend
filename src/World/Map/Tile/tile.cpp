#include "World/Map/Tile/tile.hpp"

#include "Asset/Format/adt.hpp"

#include <utility>

namespace Northrend {

MapTile::MapTile(std::shared_ptr<const AssetData> data, const bool highResolutionAlpha) :
    MapTile(Adt(std::move(data)), highResolutionAlpha) {}

MapTile::MapTile(Adt adt, const bool highResolutionAlpha) :
    terrainGeometry(adt), terrainMaterial(std::move(adt), highResolutionAlpha) {}

const TerrainGeometry& MapTile::terrain() const noexcept {
    return this->terrainGeometry;
}

const TerrainMaterial& MapTile::material() const noexcept {
    return this->terrainMaterial;
}

}
