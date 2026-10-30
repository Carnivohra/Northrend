#include "World/Map/request.hpp"

namespace Northrend {

TerrainRequest::TerrainRequest(const std::uint16_t tile, const std::uint8_t distance,
    const std::uint32_t generation) noexcept :
    tile(tile), tileDistance(distance), requestGeneration(generation) {}

std::size_t TerrainRequest::x() const noexcept {
    return this->tile % Size;
}

std::size_t TerrainRequest::y() const noexcept {
    return this->tile / Size;
}

std::size_t TerrainRequest::distance() const noexcept {
    return this->tileDistance;
}

std::uint32_t TerrainRequest::generation() const noexcept {
    return this->requestGeneration;
}

}
