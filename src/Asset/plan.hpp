#pragma once

#include "Asset/mount.hpp"

#include <vector>

namespace Northrend {

struct AssetPlan final {
    // Mount in order; later sources override earlier sources.
    std::vector<AssetMount> mounts;
};

}
