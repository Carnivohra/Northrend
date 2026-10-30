#include "World/Map/definition.hpp"

#include <utility>

namespace Northrend {

MapDefinition::MapDefinition(MapSelection selection, std::shared_ptr<const Wdt> wdt) noexcept :
    mapSelection(selection), mapData(std::move(wdt)) {}

MapDefinition::MapDefinition(MapSelection selection, std::exception_ptr failure) noexcept :
    mapSelection(selection), mapFailure(std::move(failure)) {}

const MapSelection& MapDefinition::selection() const noexcept {
    return this->mapSelection;
}

bool MapDefinition::succeeded() const noexcept {
    return static_cast<bool>(this->mapData);
}

std::shared_ptr<const Wdt> MapDefinition::data() && noexcept {
    return std::move(this->mapData);
}

void MapDefinition::rethrow_if_failed() const {
    if (this->mapFailure) {
        std::rethrow_exception(this->mapFailure);
    }
}

}
