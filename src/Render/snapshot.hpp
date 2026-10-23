#pragma once

#include <chrono>
#include <cstdint>

namespace Northrend {

struct RenderSnapshot final {
    std::uint64_t sequence = 0;
    std::chrono::nanoseconds simulationTime{};
};

}
