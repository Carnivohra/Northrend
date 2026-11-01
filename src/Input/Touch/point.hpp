#pragma once

#include <cstdint>

namespace Northrend {

struct TouchPoint final {
    std::uint64_t id = 0;
    float x = 0.0F;
    float y = 0.0F;
    float pressure = 0.0F;
    bool active = false;
};

}
