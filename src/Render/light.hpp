#pragma once

#include "Math/vector.hpp"

#include <cstdint>

namespace Northrend {

struct DirectionalLight final {
    Vector3 direction {-0.45F, -0.35F, -0.82F};
    Vector3 color {1.0F, 0.96F, 0.86F};
    float intensity = 1.0F;
    float ambient = 0.24F;
    std::uint64_t revision = 1;
};

}
