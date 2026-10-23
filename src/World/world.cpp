#include "World/world.hpp"

namespace Northrend {

Registry& World::registry() noexcept {
    return this->ecs;
}

const Registry& World::registry() const noexcept {
    return this->ecs;
}

}
