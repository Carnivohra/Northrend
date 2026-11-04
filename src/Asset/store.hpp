#pragma once

#include "Asset/plan.hpp"
#include "Asset/source.hpp"

#include <memory>
#include <stop_token>
#include <vector>

namespace Northrend {

class AssetStore final {
public:
    AssetStore() = default;
    explicit AssetStore(const AssetPlan& plan);
    AssetStore(const AssetPlan& plan, std::stop_token stopToken);

    AssetStore(const AssetStore&) = delete;
    AssetStore& operator=(const AssetStore&) = delete;
    AssetStore(AssetStore&&) = default;
    AssetStore& operator=(AssetStore&&) = default;

    void mount(std::unique_ptr<AssetSource> source);

    [[nodiscard]] std::vector<AssetId> database_assets(std::stop_token stopToken = {}) const;
    [[nodiscard]] bool contains(const AssetId& asset) const;
    [[nodiscard]] AssetData read(const AssetId& asset) const;

private:
    std::vector<std::unique_ptr<AssetSource>> sources;
};

}
