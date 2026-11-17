#pragma once

#include "Math/vector.hpp"
#include "World/Map/Object/handle.hpp"

#include <cstdint>

namespace Northrend {

struct MapObjectPlacement final {
    MapObjectHandle object;
    Vector3 position;
    Vector3 rotation;
    Vector3 minimum;
    Vector3 maximum;
    std::uint32_t id;
    std::uint16_t flags;
    std::uint16_t doodadSet;
    std::uint16_t nameSet;
    float scale;
};

}
