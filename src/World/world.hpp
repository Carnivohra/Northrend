#pragma once

#include "Thread/task.hpp"
#include "World/Map/map.hpp"
#include "World/registry.hpp"

#include <cstddef>

namespace Northrend {

class WorldExchange;

class World final {
public:
    explicit World(WorldExchange& exchange) noexcept;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = delete;

    void tick(ThreadDuration delta);

    [[nodiscard]] WorldRegistry& registry() noexcept;
    [[nodiscard]] const WorldRegistry& registry() const noexcept;
    [[nodiscard]] Map& map() noexcept;
    [[nodiscard]] const Map& map() const noexcept;

private:
    void receive_catalog();
    void receive_definition();
    void receive_terrain();
    void request_selection();
    void request_terrain();

    static constexpr std::size_t TerrainTransfersPerTick = 16;

    WorldExchange& exchange;
    WorldRegistry worldRegistry;
    Map worldMap;
};

}
