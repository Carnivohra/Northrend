#pragma once

#include "Asset/source.hpp"

#include <memory>
#include <vector>

namespace Northrend {

class AssetStorage final {
public:
    AssetStorage() = default;

    AssetStorage(const AssetStorage&) = delete;
    AssetStorage& operator=(const AssetStorage&) = delete;
    AssetStorage(AssetStorage&&) = default;
    AssetStorage& operator=(AssetStorage&&) = default;

    void mount(std::unique_ptr<AssetSource> source);

    [[nodiscard]] bool contains(const AssetId& asset) const;
    [[nodiscard]] AssetData read(const AssetId& asset) const;

private:
    std::vector<std::unique_ptr<AssetSource>> sources;
};

}
