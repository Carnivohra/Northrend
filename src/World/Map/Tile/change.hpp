#pragma once

#include "World/Map/Terrain/payload.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Northrend {

class MapTile;
class TileRequest;

class TileChange final {
public:
    enum class Kind : std::uint8_t { reset, loaded, unloaded };

    explicit TileChange(std::uint32_t generation) noexcept;
    TileChange(const TileRequest& request, std::shared_ptr<const MapTile> tile,
        std::unique_ptr<TerrainPayload> payload) noexcept;
    TileChange(std::size_t x, std::size_t y, std::uint32_t generation) noexcept;

    [[nodiscard]] Kind kind() const noexcept;
    [[nodiscard]] std::size_t x() const noexcept;
    [[nodiscard]] std::size_t y() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;
    [[nodiscard]] std::shared_ptr<const MapTile> tile() && noexcept;
    [[nodiscard]] std::unique_ptr<TerrainPayload> payload() && noexcept;

private:
    std::shared_ptr<const MapTile> mapTile;
    std::unique_ptr<TerrainPayload> terrainPayload;
    std::uint32_t tileGeneration;
    std::uint8_t tileX = 0;
    std::uint8_t tileY = 0;
    Kind changeKind;
};

}
