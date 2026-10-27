#pragma once

#include <cstdint>

namespace Northrend {

struct WorldSnapshot final {
    std::uint64_t sequence = 0;
};

}
