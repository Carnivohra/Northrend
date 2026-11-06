#include "World/Map/Tile/result.hpp"

#include <utility>

namespace Northrend {

TileResult::TileResult(
    TileRequest request, std::shared_ptr<const MapTile> tile, std::unique_ptr<TerrainPayload> payload) noexcept :
    tileRequest(request), mapTile(std::move(tile)), terrainPayload(std::move(payload)) {}

TileResult::TileResult(TileRequest request, std::exception_ptr failure) noexcept :
    tileRequest(request), tileFailure(std::move(failure)) {}

const TileRequest& TileResult::request() const noexcept {
    return this->tileRequest;
}

bool TileResult::succeeded() const noexcept {
    return static_cast<bool>(this->mapTile);
}

const std::shared_ptr<const MapTile>& TileResult::data() const noexcept {
    return this->mapTile;
}

std::shared_ptr<const MapTile> TileResult::data() && noexcept {
    return std::move(this->mapTile);
}

std::unique_ptr<TerrainPayload> TileResult::payload() && noexcept {
    return std::move(this->terrainPayload);
}

void TileResult::rethrow_if_failed() const {
    if (this->tileFailure) {
        std::rethrow_exception(this->tileFailure);
    }
}

}
