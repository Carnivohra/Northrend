#pragma once

#include "World/Map/Terrain/geometry.hpp"
#include "World/Map/Terrain/material.hpp"

#include <memory>

namespace Northrend {

class AssetData;

class MapTile final {
public:
    MapTile(std::shared_ptr<const AssetData> data, bool highResolutionAlpha);

    [[nodiscard]] const TerrainGeometry& terrain() const noexcept;
    [[nodiscard]] const TerrainMaterial& material() const noexcept;

private:
    MapTile(Adt adt, bool highResolutionAlpha);

    TerrainGeometry terrainGeometry;
    TerrainMaterial terrainMaterial;
};

}
