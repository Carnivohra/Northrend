#pragma once

#include "Thread/task.hpp"
#include "World/Map/map.hpp"
#include "World/Map/Tile/change.hpp"
#include "World/registry.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace Northrend {

class MapExchange;

class World final {
public:
    explicit World(MapExchange& exchange) noexcept;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = delete;

    void tick(ThreadDuration delta);
    [[nodiscard]] std::optional<TileChange> next_tile_change();

    [[nodiscard]] WorldRegistry& registry() noexcept;
    [[nodiscard]] const WorldRegistry& registry() const noexcept;
    [[nodiscard]] Map& map() noexcept;
    [[nodiscard]] const Map& map() const noexcept;

private:
    void receive_catalog();
    void receive_map();
    void receive_tiles();
    void request_selection();
    void request_tiles();

    static constexpr std::size_t TileChangesPerTick = 16;

    MapExchange& exchange;
    WorldRegistry worldRegistry;
    Map worldMap;
    std::vector<TileChange> tileChanges;
    std::size_t tileChangePosition = 0;
};

}
