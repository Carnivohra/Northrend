#include "Asset/Stream/request.hpp"

#include "Asset/Stream/state.hpp"

#include <stdexcept>
#include <utility>

namespace Northrend {

AssetRequest::AssetRequest(std::shared_ptr<AssetState> state) noexcept : state(std::move(state)) {}

bool AssetRequest::valid() const noexcept {
    return static_cast<bool>(this->state);
}

bool AssetRequest::ready() const {
    return this->state && this->state->ready();
}

std::shared_ptr<const AssetData> AssetRequest::data() const {
    if (!this->state) {
        throw std::logic_error("Asset request is empty");
    }

    return this->state->data();
}

}
