#pragma once

#include "Input/Gamepad/axis.hpp"
#include "Input/Gamepad/button.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <utility>

namespace Northrend {

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

    [[nodiscard]] std::pair<float, float> stick(
        GamepadAxis horizontal, GamepadAxis vertical, float deadzone = 0.15F) const noexcept {
        const float x = this->axis(horizontal);
        const float y = this->axis(vertical);
        const float length = std::sqrt(x * x + y * y);
        const float threshold = std::clamp(deadzone, 0.0F, 0.99F);

        if (length <= threshold || length == 0.0F) {
            return {};
        }

        const float magnitude = std::min((length - threshold) / (1.0F - threshold), 1.0F);
        return {x * magnitude / length, y * magnitude / length};
    }

    void connect() noexcept {
        this->available = true;
    }

    void disconnect() noexcept {
        this->available = false;
        this->neutralize();
    }

    void press(GamepadButton button) noexcept {
        const std::size_t index = static_cast<std::size_t>(button);

        if (index < ButtonCount) {
            this->held.set(index);
        }
    }

    void release(GamepadButton button) noexcept {
        const std::size_t index = static_cast<std::size_t>(button);

        if (index < ButtonCount) {
            this->held.reset(index);
        }
    }

    void move(GamepadAxis axis, float value) noexcept {
        const std::size_t index = static_cast<std::size_t>(axis);

        if (index < AxisCount) {
            this->axes[index] = std::clamp(value, -1.0F, 1.0F);
        }
    }

    void neutralize() noexcept {
        this->held.reset();
        this->axes.fill(0.0F);
    }

    void reset() noexcept {
        this->disconnect();
    }

private:
    static constexpr std::size_t ButtonCount = static_cast<std::size_t>(GamepadButton::count);
    static constexpr std::size_t AxisCount = static_cast<std::size_t>(GamepadAxis::count);

    bool available = false;
    std::bitset<ButtonCount> held;
    std::array<float, AxisCount> axes {};
};

}
