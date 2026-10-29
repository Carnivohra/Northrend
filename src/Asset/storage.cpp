#include "Asset/storage.hpp"

#include "Asset/directory.hpp"
#include "Asset/mpq.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

std::vector<AssetId> AssetStorage::database_assets(const std::stop_token stopToken) const {
    std::unordered_map<std::string, AssetId> unique;

    for (const std::unique_ptr<AssetSource>& source : this->sources) {
        if (stopToken.stop_requested()) {
            return {};
        }

        for (AssetId& asset : source->entries(stopToken)) {
            if (stopToken.stop_requested()) {
                return {};
            }

            const std::string_view path = asset.path();

            if (path.size() < 4 || path[path.size() - 4] != '.') {
                continue;
            }

            const auto lowercase = [](const char value) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
            };
            const bool database = lowercase(path[path.size() - 3]) == 'd' &&
                lowercase(path[path.size() - 2]) == 'b' &&
                (lowercase(path[path.size() - 1]) == 'c' || lowercase(path[path.size() - 1]) == '2');

            if (!database) {
                continue;
            }

            std::string key(path);
            std::ranges::transform(key, key.begin(), lowercase);
            unique.insert_or_assign(key, std::move(asset));
        }
    }

    std::vector<AssetId> assets;
    assets.reserve(unique.size());

    for (auto& [key, asset] : unique) {
        assets.push_back(std::move(asset));
    }

    std::ranges::sort(assets, [](const AssetId& left, const AssetId& right) {
        return left.path() < right.path();
    });
    return assets;
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
