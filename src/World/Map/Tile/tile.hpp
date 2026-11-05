#pragma once

#include "World/Map/Terrain/geometry.hpp"

#include <memory>

namespace Northrend {

class AssetData;

class MapTile final {
public:
    explicit MapTile(std::shared_ptr<const AssetData> data);

    [[nodiscard]] const TerrainGeometry& terrain() const noexcept;

private:
    TerrainGeometry terrainGeometry;
};

}
