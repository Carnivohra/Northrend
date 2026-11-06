#pragma once

#include "Asset/plan.hpp"
#include "Config/plan.hpp"

#include <cstddef>
#include <optional>

namespace Northrend {

struct EnginePlan final {
    std::optional<ConfigPlan> config;
    AssetPlan assets;
    std::size_t renderDistance = 12;
};

}
