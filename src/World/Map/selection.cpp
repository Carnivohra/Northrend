#include "World/Map/selection.hpp"

namespace Northrend {

MapSelection::MapSelection(const std::uint32_t id, const std::uint32_t generation) noexcept :
    identifier(id), selectionGeneration(generation) {}

std::uint32_t MapSelection::id() const noexcept {
    return this->identifier;
}

std::uint32_t MapSelection::generation() const noexcept {
    return this->selectionGeneration;
}

}
