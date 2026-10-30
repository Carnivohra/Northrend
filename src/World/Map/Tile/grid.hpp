#pragma once

#include "World/Map/Tile/request.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace Northrend {

class Map;
class MapLayout;
class MapTile;
class TileResult;

enum class TileState : std::uint8_t { missing, requested, loading, ready, active, failed };

class TileGrid final {
public:
    static constexpr std::size_t Size = 64;
    static constexpr std::size_t TileCount = Size * Size;

    TileGrid() = default;
    TileGrid(std::uint32_t generation, std::shared_ptr<const MapLayout> layout) noexcept;

    void load_all() noexcept;
    void focus(std::size_t x, std::size_t y, std::size_t radius);
    [[nodiscard]] std::optional<TileRequest> next_request() noexcept;
    [[nodiscard]] bool defer(const TileRequest& request) noexcept;

    [[nodiscard]] bool complete(TileResult result) noexcept;
    [[nodiscard]] bool fail(const TileRequest& request) noexcept;
    [[nodiscard]] bool activate(std::size_t x, std::size_t y) noexcept;
    [[nodiscard]] bool deactivate(std::size_t x, std::size_t y) noexcept;
    void release(std::size_t x, std::size_t y) noexcept;
    void reset() noexcept;

    [[nodiscard]] TileState state(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] const MapTile* tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] std::size_t pending() const noexcept;

private:
    friend class Map;

    [[nodiscard]] std::uint32_t generation() const noexcept;
    [[nodiscard]] static constexpr std::uint16_t index(const std::size_t x, const std::size_t y) noexcept {
        return static_cast<std::uint16_t>(y * Size + x);
    }

    void enqueue(std::size_t x, std::size_t y, std::size_t distance) noexcept;

    std::array<TileState, TileCount> states {};
    std::array<std::uint16_t, TileCount> requests {};
    std::array<std::uint8_t, TileCount> requestDistances {};
    std::array<std::shared_ptr<const MapTile>, TileCount> tiles;
    std::shared_ptr<const MapLayout> mapLayout;
    std::size_t requestPosition = 0;
    std::size_t requestCount = 0;
    std::uint32_t currentGeneration = 1;
};

}
