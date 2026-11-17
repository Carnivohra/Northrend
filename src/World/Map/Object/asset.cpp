#include "World/Map/Object/asset.hpp"

#include <stdexcept>
#include <utility>

namespace Northrend {

MapObjectAsset::MapObjectAsset(std::unique_ptr<Wmo> root, std::vector<WmoGroup> groups) :
    rootData(std::move(root)), groupData(std::move(groups)) {
    if (!this->rootData) {
        throw std::invalid_argument("Map object root is empty");
    }

    if (this->groupData.size() != this->rootData->header().groupCount) {
        throw std::invalid_argument("Map object group count does not match its root");
    }
}

const Wmo& MapObjectAsset::root() const noexcept {
    return *this->rootData;
}

std::span<const WmoGroup> MapObjectAsset::groups() const noexcept {
    return this->groupData;
}

}
