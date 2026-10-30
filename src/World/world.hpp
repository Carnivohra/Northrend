#pragma once

#include "World/Map/map.hpp"
#include "World/registry.hpp"

namespace Northrend {

class World final {
public:
    World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;

    [[nodiscard]] WorldRegistry& registry() noexcept;
    [[nodiscard]] const WorldRegistry& registry() const noexcept;
    [[nodiscard]] Map& map() noexcept;
    [[nodiscard]] const Map& map() const noexcept;

private:
    WorldRegistry worldRegistry;
    Map worldMap;
};

}
