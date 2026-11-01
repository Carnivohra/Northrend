#pragma once

#include <array>
#include <bitset>
#include <cstddef>
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

enum class GamepadAxis : std::uint8_t { leftX, leftY, rightX, rightY, leftTrigger, rightTrigger, count };

class Gamepad final {
public:
    [[nodiscard]] bool connected() const noexcept {
        return this->available;
    }

    [[nodiscard]] bool down(GamepadButton button) const noexcept {
        const std::size_t index = static_cast<std::size_t>(button);
        return index < ButtonCount && this->held.test(index);
    }

    [[nodiscard]] float axis(GamepadAxis axis) const noexcept {
        const std::size_t index = static_cast<std::size_t>(axis);
        return index < AxisCount ? this->axes[index] : 0.0F;
    }

    void reset() noexcept {
        this->available = false;
        this->held.reset();
        this->axes.fill(0.0F);
    }

private:
    static constexpr std::size_t ButtonCount = static_cast<std::size_t>(GamepadButton::count);
    static constexpr std::size_t AxisCount = static_cast<std::size_t>(GamepadAxis::count);

    bool available = false;
    std::bitset<ButtonCount> held;
    std::array<float, AxisCount> axes {};
};

}
