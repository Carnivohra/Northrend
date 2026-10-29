#include "Asset/Stream/state.hpp"

#include <stdexcept>
#include <utility>

namespace Northrend {

AssetState::AssetState(AssetId asset, AssetPriority priority) :
    identifier(std::move(asset)), requestPriority(priority) {}

const AssetId& AssetState::asset() const noexcept {
    return this->identifier;
}

AssetPriority AssetState::priority() const noexcept {
    return this->requestPriority;
}

void AssetState::set_priority(AssetPriority priority) noexcept {
    this->requestPriority = priority;
}

bool AssetState::queued() const {
    const std::scoped_lock lock(this->mutex);
    return this->status == Status::queued;
}

bool AssetState::begin() {
    const std::scoped_lock lock(this->mutex);

    if (this->status != Status::queued) {
        return false;
    }

    this->status = Status::loading;
    return true;
}

void AssetState::complete(std::shared_ptr<const AssetData> data) {
    {
        const std::scoped_lock lock(this->mutex);

        if (this->status != Status::loading) {
            return;
        }

        this->result = std::move(data);
        this->status = Status::ready;
    }

    this->wake.notify_all();
}

void AssetState::fail(std::exception_ptr failure) {
    {
        const std::scoped_lock lock(this->mutex);

        if (this->status != Status::loading) {
            return;
        }

        this->failure = std::move(failure);
        this->status = Status::failed;
    }

    this->wake.notify_all();
}

void AssetState::cancel() {
    {
        const std::scoped_lock lock(this->mutex);

        if (this->status != Status::queued) {
            return;
        }

        this->status = Status::cancelled;
    }

    this->wake.notify_all();
}

bool AssetState::ready() const {
    const std::scoped_lock lock(this->mutex);
    return this->status == Status::ready || this->status == Status::failed || this->status == Status::cancelled;
}

std::shared_ptr<const AssetData> AssetState::data() const {
    std::unique_lock lock(this->mutex);
    this->wake.wait(lock, [this] {
        return this->status == Status::ready || this->status == Status::failed || this->status == Status::cancelled;
    });

    if (this->status == Status::failed) {
        std::rethrow_exception(this->failure);
    }

    if (this->status == Status::cancelled) {
        throw std::runtime_error("Asset request was cancelled");
    }

    return this->result;
}

}
