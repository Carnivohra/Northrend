#include "World/Map/Object/handle.hpp"

#include "World/Map/Object/state.hpp"

#include <utility>

namespace Northrend {

MapObjectHandle::MapObjectHandle(std::shared_ptr<MapObjectState> state) noexcept : state(std::move(state)) {}

bool MapObjectHandle::valid() const noexcept {
    return static_cast<bool>(this->state);
}

bool MapObjectHandle::ready() const noexcept {
    return this->state && this->state->ready();
}

std::string_view MapObjectHandle::asset() const noexcept {
    return this->state ? this->state->asset().path() : std::string_view {};
}

std::shared_ptr<const MapObjectAsset> MapObjectHandle::data() const {
    return this->state ? this->state->data() : nullptr;
}

}
