#pragma once

#include <cstdint>

namespace Northrend {

enum class GamepadButton : std::uint8_t {
    south,
    east,
    west,
    north,
    back,
    guide,
    start,
    leftStick,
    rightStick,
    leftShoulder,
    rightShoulder,
    up,
    down,
    left,
    right,
    count
};

}
