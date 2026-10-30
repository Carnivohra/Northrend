#include "World/Map/Tile/change.hpp"

#include "World/Map/Tile/request.hpp"

#include <utility>

namespace Northrend {

TileChange::TileChange(const std::uint32_t generation) noexcept :
    tileGeneration(generation), changeKind(Kind::reset) {}

TileChange::TileChange(const TileRequest& request, std::shared_ptr<const MapTile> tile) noexcept :
    mapTile(std::move(tile)),
    tileGeneration(request.generation()),
    tileX(static_cast<std::uint8_t>(request.x())),
    tileY(static_cast<std::uint8_t>(request.y())),
    changeKind(Kind::loaded) {}

TileChange::TileChange(
    const std::size_t x, const std::size_t y, const std::uint32_t generation) noexcept :
    tileGeneration(generation),
    tileX(static_cast<std::uint8_t>(x)),
    tileY(static_cast<std::uint8_t>(y)),
    changeKind(Kind::unloaded) {}

TileChange::Kind TileChange::kind() const noexcept {
    return this->changeKind;
}

std::size_t TileChange::x() const noexcept {
    return this->tileX;
}

std::size_t TileChange::y() const noexcept {
    return this->tileY;
}

std::uint32_t TileChange::generation() const noexcept {
    return this->tileGeneration;
}

std::shared_ptr<const MapTile> TileChange::tile() && noexcept {
    return std::move(this->mapTile);
}

}
