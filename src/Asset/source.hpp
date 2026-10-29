#pragma once

#include "Asset/data.hpp"
#include "Asset/id.hpp"

#include <optional>
#include <stop_token>
#include <vector>

namespace Northrend {

class AssetSource {
public:
    virtual ~AssetSource() = default;

    [[nodiscard]] virtual std::vector<AssetId> entries(std::stop_token stopToken = {}) const = 0;
    [[nodiscard]] virtual bool contains(const AssetId& asset) const = 0;
    [[nodiscard]] virtual std::optional<AssetData> try_read(const AssetId& asset) const = 0;
};

}
