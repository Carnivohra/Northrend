#pragma once

#include "World/Map/Terrain/request.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace Northrend {

class Map;
class TerrainResult;
class TerrainTile;
class Wdt;

enum class TerrainTileState : std::uint8_t { missing, requested, loading, ready, active, failed };

class Terrain final {
public:
    static constexpr std::size_t Size = 64;
    static constexpr std::size_t TileCount = Size * Size;

    Terrain() = default;
    Terrain(std::uint32_t generation, std::shared_ptr<const Wdt> definition) noexcept;

    void focus(std::size_t x, std::size_t y, std::size_t radius);
    [[nodiscard]] std::optional<TerrainRequest> next_request() noexcept;
    [[nodiscard]] bool defer(const TerrainRequest& request) noexcept;

    [[nodiscard]] bool complete(TerrainResult result) noexcept;
    [[nodiscard]] bool fail(const TerrainRequest& request) noexcept;
    [[nodiscard]] bool activate(std::size_t x, std::size_t y) noexcept;
    [[nodiscard]] bool deactivate(std::size_t x, std::size_t y) noexcept;
    void release(std::size_t x, std::size_t y) noexcept;
    void reset() noexcept;

    [[nodiscard]] TerrainTileState state(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] const TerrainTile* tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] std::size_t pending() const noexcept;

private:
    friend class Map;

    [[nodiscard]] std::uint32_t generation() const noexcept;
    [[nodiscard]] static constexpr std::uint16_t index(const std::size_t x, const std::size_t y) noexcept {
        return static_cast<std::uint16_t>(y * Size + x);
    }

    void enqueue(std::size_t x, std::size_t y, std::size_t distance) noexcept;

    std::array<TerrainTileState, TileCount> states {};
    std::array<std::uint16_t, TileCount> requests {};
    std::array<std::uint8_t, TileCount> requestDistances {};
    std::array<std::shared_ptr<const TerrainTile>, TileCount> tiles;
    std::shared_ptr<const Wdt> definition;
    std::size_t requestPosition = 0;
    std::size_t requestCount = 0;
    std::uint32_t currentGeneration = 1;
};

}
