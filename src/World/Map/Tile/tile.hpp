#pragma once

#include "Asset/Stream/priority.hpp"
#include "World/Map/Object/placement.hpp"
#include "World/Map/Terrain/geometry.hpp"
#include "World/Map/Terrain/material.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace Northrend {

class MapObjectLoader;

class MapTile final {
public:
    MapTile(const Adt& adt, MapObjectLoader& objects, AssetPriority priority, std::uint32_t generation);

    [[nodiscard]] const TerrainGeometry& terrain() const noexcept;
    [[nodiscard]] const TerrainMaterial& material() const noexcept;
    [[nodiscard]] std::span<const MapObjectPlacement> objects() const noexcept;

private:
    TerrainGeometry terrainGeometry;
    TerrainMaterial terrainMaterial;
    std::vector<MapObjectPlacement> mapObjects;
};

}
