#include "Asset/storage.hpp"

#include "Asset/directory.hpp"
#include "Asset/mpq.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

AssetStorage::AssetStorage(const AssetPlan& plan) {
    this->sources.reserve(plan.mounts.size());

    for (const AssetMount& mount : plan.mounts) {
        switch (mount.type) {
        case AssetMount::Type::directory:
            this->mount(std::make_unique<AssetDirectory>(mount.path));
            break;
        case AssetMount::Type::mpq:
            this->mount(std::make_unique<MpqArchive>(mount.path));
            break;
        }
    }
}

void AssetStorage::mount(std::unique_ptr<AssetSource> source) {
    if (!source) {
        throw std::invalid_argument("Cannot mount an empty asset source");
    }

    this->sources.push_back(std::move(source));
}

bool AssetStorage::contains(const AssetId& asset) const {
    for (auto source = this->sources.rbegin(); source != this->sources.rend(); ++source) {
        if ((*source)->contains(asset)) {
            return true;
        }
    }

    return false;
}

AssetData AssetStorage::read(const AssetId& asset) const {
    for (auto source = this->sources.rbegin(); source != this->sources.rend(); ++source) {
        if ((*source)->contains(asset)) {
            return (*source)->read(asset);
        }
    }

    throw std::runtime_error("Asset was not found: " + std::string(asset.path()));
}

}
