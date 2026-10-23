#pragma once

#include "ECS/registry.hpp"

namespace Northrend {

class World final {
public:
    World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;

    [[nodiscard]] Registry& registry() noexcept;
    [[nodiscard]] const Registry& registry() const noexcept;

private:
    Registry ecs;
};

}
