#include "World/Map/Object/state.hpp"

#include "World/Map/Object/asset.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace Northrend {

MapObjectState::MapObjectState(
    AssetId asset, const AssetPriority priority, const std::uint32_t generation) :
    identifier(std::move(asset)), requestPriority(priority), mapGeneration(generation) {}

const AssetId& MapObjectState::asset() const noexcept {
    return this->identifier;
}

std::uint32_t MapObjectState::generation() const noexcept {
    return this->mapGeneration;
}

AssetPriority MapObjectState::priority() const noexcept {
    const std::scoped_lock lock(this->mutex);
    return this->requestPriority;
}

void MapObjectState::promote(const AssetPriority priority) noexcept {
    const std::scoped_lock lock(this->mutex);

    if (static_cast<std::uint8_t>(priority) > static_cast<std::uint8_t>(this->requestPriority)) {
        this->requestPriority = priority;
    }
}

bool MapObjectState::ready() const noexcept {
    const std::scoped_lock lock(this->mutex);
    return this->status != Status::loading;
}

std::shared_ptr<const MapObjectAsset> MapObjectState::data() const {
    const std::scoped_lock lock(this->mutex);

    if (this->status == Status::failed) {
        std::rethrow_exception(this->failure);
    }

    if (this->status == Status::cancelled) {
        return nullptr;
    }

    return this->result;
}

void MapObjectState::complete(std::shared_ptr<const MapObjectAsset> data) {
    if (!data) {
        throw std::invalid_argument("Map object result is empty");
    }

    const std::scoped_lock lock(this->mutex);

    if (this->status != Status::loading) {
        return;
    }

    this->result = std::move(data);
    this->status = Status::ready;
}

void MapObjectState::fail(std::exception_ptr failure) noexcept {
    const std::scoped_lock lock(this->mutex);

    if (this->status != Status::loading) {
        return;
    }

    this->failure = std::move(failure);
    this->status = Status::failed;
}

void MapObjectState::cancel() noexcept {
    const std::scoped_lock lock(this->mutex);

    if (this->status == Status::loading) {
        this->status = Status::cancelled;
    }
}

}
