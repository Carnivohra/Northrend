#pragma once

#include "Asset/mount.hpp"

#include <vector>

namespace Northrend {

struct AssetPlan final {
    std::vector<AssetMount> mounts;
};

}
