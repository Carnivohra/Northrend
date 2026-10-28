#pragma once

#include "Asset/plan.hpp"
#include "Config/plan.hpp"

#include <optional>

namespace Northrend {

struct GamePlan final {
    std::optional<ConfigPlan> config;
    AssetPlan assets;
};

}
