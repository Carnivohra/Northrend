#pragma once

#include "Input/gamepad.hpp"
#include "Input/keyboard.hpp"
#include "Input/mouse.hpp"
#include "Input/touch.hpp"

namespace Northrend {

class Input final {
public:
    Input() = default;

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&) = delete;
    Input& operator=(Input&&) = delete;

    [[nodiscard]] Keyboard& keyboard() noexcept {
        return this->keyboardState;
    }

    [[nodiscard]] const Keyboard& keyboard() const noexcept {
        return this->keyboardState;
    }

    [[nodiscard]] Mouse& mouse() noexcept {
        return this->mouseState;
    }

    [[nodiscard]] const Mouse& mouse() const noexcept {
        return this->mouseState;
    }

    [[nodiscard]] Touch& touch() noexcept {
        return this->touchState;
    }

    [[nodiscard]] const Touch& touch() const noexcept {
        return this->touchState;
    }

    [[nodiscard]] Gamepad& gamepad() noexcept {
        return this->gamepadState;
    }

    [[nodiscard]] const Gamepad& gamepad() const noexcept {
        return this->gamepadState;
    }

    void begin_frame() noexcept {
        this->keyboardState.begin_frame();
        this->mouseState.begin_frame();
    }

    void reset() noexcept {
        this->keyboardState.reset();
        this->mouseState.reset();
        this->touchState.reset();
        this->gamepadState.reset();
    }

private:
    Keyboard keyboardState;
    Mouse mouseState;
    Touch touchState;
    Gamepad gamepadState;
};

}
