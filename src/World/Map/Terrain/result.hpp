#pragma once

#include "World/Map/Terrain/request.hpp"

#include <exception>
#include <memory>

namespace Northrend {

class TerrainTile;

class TerrainResult final {
public:
    TerrainResult(TerrainRequest request, std::shared_ptr<const TerrainTile> tile) noexcept;
    TerrainResult(TerrainRequest request, std::exception_ptr failure) noexcept;

    [[nodiscard]] const TerrainRequest& request() const noexcept;
    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] std::shared_ptr<const TerrainTile> data() && noexcept;
    void rethrow_if_failed() const;

private:
    TerrainRequest terrainRequest;
    std::shared_ptr<const TerrainTile> terrainData;
    std::exception_ptr terrainFailure;
};

}
