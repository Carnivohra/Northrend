#pragma once

#include "Assets/asset.hpp"

namespace Northrend {

class AssetSource {
public:
    virtual ~AssetSource() = default;

    [[nodiscard]] virtual bool contains(const AssetId& asset) const = 0;
    [[nodiscard]] virtual AssetData read(const AssetId& asset) const = 0;
};

}
