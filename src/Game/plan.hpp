#pragma once

#include "Asset/plan.hpp"
#include "Config/plan.hpp"

#include <optional>
#include <string>

namespace Northrend {

struct GamePlan final {
    std::string name;
    std::optional<ConfigPlan> config;
    AssetPlan assets;
};

}
