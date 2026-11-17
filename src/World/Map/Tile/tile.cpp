#include "World/Map/Tile/tile.hpp"

#include "World/Map/Object/loader.hpp"

namespace Northrend {

MapTile::MapTile(const Adt& adt, MapObjectLoader& objects, const AssetPriority priority,
    const std::uint32_t generation) : terrainGeometry(adt), terrainMaterial(adt) {
    this->mapObjects.reserve(adt.wmo_placements().size());

    for (const Adt::WmoPlacement& placement : adt.wmo_placements()) {
        this->mapObjects.push_back(MapObjectPlacement {
            .object = objects.reference(placement.asset, priority, generation),
            .position = placement.position,
            .rotation = placement.rotation,
            .minimum = placement.minimum,
            .maximum = placement.maximum,
            .id = placement.id,
            .flags = placement.flags,
            .doodadSet = placement.doodadSet,
            .nameSet = placement.nameSet,
            .scale = placement.scale
        });
    }
}

const TerrainGeometry& MapTile::terrain() const noexcept {
    return this->terrainGeometry;
}

const TerrainMaterial& MapTile::material() const noexcept {
    return this->terrainMaterial;
}

std::span<const MapObjectPlacement> MapTile::objects() const noexcept {
    return this->mapObjects;
}

}
