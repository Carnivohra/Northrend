#pragma once

#include "Backend/plan.hpp"
#include "Engine/plan.hpp"

namespace Northrend {

struct AppPlan final {
    BackendPlan backend;
    EnginePlan engine;
};

}
