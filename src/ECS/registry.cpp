#include "ECS/registry.hpp"

namespace Northrend {

Entity Registry::create() {
    return Entity(this->world.entity().id());
}

void Registry::destroy(Entity entity) {
    this->world.entity(entity.id()).destruct();
}

bool Registry::alive(Entity entity) const noexcept {
    return this->world.is_alive(entity.id());
}

}
