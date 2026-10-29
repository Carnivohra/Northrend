#include "Asset/storage.hpp"

#include "Asset/directory.hpp"
#include "Asset/mpq.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace Northrend {

AssetStorage::AssetStorage(const AssetPlan& plan) : AssetStorage(plan, {}) {}

AssetStorage::AssetStorage(const AssetPlan& plan, const std::stop_token stopToken) {
    this->sources.reserve(plan.mounts.size());

    for (const AssetMount& mount : plan.mounts) {
        if (stopToken.stop_requested()) {
            return;
        }

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
        std::optional<AssetData> data = (*source)->try_read(asset);

        if (data) {
            return std::move(*data);
        }
    }

    throw std::runtime_error("Asset was not found: " + std::string(asset.path()));
}

}
