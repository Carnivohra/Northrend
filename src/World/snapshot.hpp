#pragma once

#include "Math/vector.hpp"

#include <cstdint>
#include <optional>

namespace Northrend {

struct WorldSnapshot final {
    std::uint64_t sequence = 0;
    std::uint32_t mapGeneration = 0;
    std::optional<Vector3> mapCenter;
};

}
