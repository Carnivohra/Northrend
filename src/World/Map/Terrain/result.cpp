#include "World/Map/Terrain/result.hpp"

#include <utility>

namespace Northrend {

TerrainResult::TerrainResult(TerrainRequest request, std::shared_ptr<const TerrainTile> tile) noexcept :
    terrainRequest(request), terrainData(std::move(tile)) {}

TerrainResult::TerrainResult(TerrainRequest request, std::exception_ptr failure) noexcept :
    terrainRequest(request), terrainFailure(std::move(failure)) {}

const TerrainRequest& TerrainResult::request() const noexcept {
    return this->terrainRequest;
}

bool TerrainResult::succeeded() const noexcept {
    return static_cast<bool>(this->terrainData);
}

std::shared_ptr<const TerrainTile> TerrainResult::data() && noexcept {
    return std::move(this->terrainData);
}

void TerrainResult::rethrow_if_failed() const {
    if (this->terrainFailure) {
        std::rethrow_exception(this->terrainFailure);
    }
}

}
