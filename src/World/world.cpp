#include "World/world.hpp"

namespace Northrend {

WorldRegistry& World::registry() noexcept {
    return this->worldRegistry;
}

const WorldRegistry& World::registry() const noexcept {
    return this->worldRegistry;
}

Map& World::map() noexcept {
    return this->worldMap;
}

const Map& World::map() const noexcept {
    return this->worldMap;
}

}
