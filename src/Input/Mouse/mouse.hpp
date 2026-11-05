#pragma once

#include "Input/Mouse/button.hpp"

#include <bitset>
#include <cstddef>

namespace Northrend {

class Mouse final {
public:
    [[nodiscard]] bool down(MouseButton button) const noexcept {
        return valid(button) && this->held.test(index(button));
    }

    [[nodiscard]] bool pressed(MouseButton button) const noexcept {
        return valid(button) && this->pressedThisFrame.test(index(button));
    }

    [[nodiscard]] bool released(MouseButton button) const noexcept {
        return valid(button) && this->releasedThisFrame.test(index(button));
    }

    [[nodiscard]] float x() const noexcept {
        return this->positionX;
    }

    [[nodiscard]] float y() const noexcept {
        return this->positionY;
    }

    [[nodiscard]] float delta_x() const noexcept {
        return this->deltaX;
    }

    [[nodiscard]] float delta_y() const noexcept {
        return this->deltaY;
    }

    [[nodiscard]] float wheel_x() const noexcept {
        return this->wheelX;
    }

    [[nodiscard]] float wheel_y() const noexcept {
        return this->wheelY;
    }

    void press(MouseButton button) noexcept {
        if (!valid(button)) {
            return;
        }

        const std::size_t position = index(button);

        if (!this->held.test(position)) {
            this->held.set(position);
            this->pressedThisFrame.set(position);
        }
    }

    void release(MouseButton button) noexcept {
        if (!valid(button)) {
            return;
        }

        const std::size_t position = index(button);

        if (this->held.test(position)) {
            this->held.reset(position);
            this->releasedThisFrame.set(position);
        }
    }

    void move(float x, float y, float deltaX, float deltaY) noexcept {
        this->positionX = x;
        this->positionY = y;
        this->deltaX += deltaX;
        this->deltaY += deltaY;
    }

    void scroll(float x, float y) noexcept {
        this->wheelX += x;
        this->wheelY += y;
    }

    void begin_frame() noexcept {
        this->pressedThisFrame.reset();
        this->releasedThisFrame.reset();
        this->deltaX = 0.0F;
        this->deltaY = 0.0F;
        this->wheelX = 0.0F;
        this->wheelY = 0.0F;
    }

    void reset() noexcept {
        this->releasedThisFrame |= this->held;
        this->held.reset();
        this->pressedThisFrame.reset();
        this->deltaX = 0.0F;
        this->deltaY = 0.0F;
        this->wheelX = 0.0F;
        this->wheelY = 0.0F;
    }

private:
    static constexpr std::size_t Count = static_cast<std::size_t>(MouseButton::count);

    [[nodiscard]] static constexpr std::size_t index(MouseButton button) noexcept {
        return static_cast<std::size_t>(button);
    }

    [[nodiscard]] static constexpr bool valid(MouseButton button) noexcept {
        return index(button) < Count;
    }

    std::bitset<Count> held;
    std::bitset<Count> pressedThisFrame;
    std::bitset<Count> releasedThisFrame;
    float positionX = 0.0F;
    float positionY = 0.0F;
    float deltaX = 0.0F;
    float deltaY = 0.0F;
    float wheelX = 0.0F;
    float wheelY = 0.0F;
};

}
