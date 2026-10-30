#pragma once

#include "World/Map/Terrain/tile.hpp"

#include <memory>

namespace Northrend {

class AssetData;

class MapTile final {
public:
    explicit MapTile(std::shared_ptr<const AssetData> data);

    [[nodiscard]] const TerrainTile& terrain() const noexcept;

private:
    TerrainTile mapTerrain;
};

}
