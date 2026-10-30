#include "World/Map/Tile/request.hpp"

namespace Northrend {

TileRequest::TileRequest(
    const std::uint16_t tile, const std::uint8_t distance, const std::uint32_t generation) noexcept :
    tile(tile), tileDistance(distance), requestGeneration(generation) {}

std::size_t TileRequest::x() const noexcept {
    return this->tile % Size;
}

std::size_t TileRequest::y() const noexcept {
    return this->tile / Size;
}

std::size_t TileRequest::distance() const noexcept {
    return this->tileDistance;
}

std::uint32_t TileRequest::generation() const noexcept {
    return this->requestGeneration;
}

}
