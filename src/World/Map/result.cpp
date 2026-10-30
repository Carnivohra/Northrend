#include "World/Map/result.hpp"

#include <utility>

namespace Northrend {

MapResult::MapResult(MapSelection selection, std::shared_ptr<const MapLayout> layout) noexcept :
    mapSelection(selection), mapLayout(std::move(layout)) {}

MapResult::MapResult(MapSelection selection, std::exception_ptr failure) noexcept :
    mapSelection(selection), mapFailure(std::move(failure)) {}

const MapSelection& MapResult::selection() const noexcept {
    return this->mapSelection;
}

bool MapResult::succeeded() const noexcept {
    return static_cast<bool>(this->mapLayout);
}

std::shared_ptr<const MapLayout> MapResult::data() && noexcept {
    return std::move(this->mapLayout);
}

void MapResult::rethrow_if_failed() const {
    if (this->mapFailure) {
        std::rethrow_exception(this->mapFailure);
    }
}

}
