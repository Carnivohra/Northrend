#pragma once

#include "Input/Gamepad/axis.hpp"
#include "Input/Gamepad/button.hpp"
#include "Input/Keyboard/keyboard.hpp"
#include "Input/Mouse/mouse.hpp"

#include <cstdint>
#include <optional>

union SDL_Event;
struct SDL_Gamepad;

namespace Northrend {

class Input;

class SDL3Input final {
public:
    void process(const SDL_Event& event, Input& input) noexcept;
    void close() noexcept;

private:
    void open(std::uint32_t id, Input& input) noexcept;
    void open_available(Input& input) noexcept;
    void remove(std::uint32_t id, Input& input) noexcept;
    [[nodiscard]] bool active(std::uint32_t id) const noexcept;
    [[nodiscard]] static std::optional<GamepadAxis> gamepad_axis(std::uint8_t axis) noexcept;
    [[nodiscard]] static std::optional<GamepadButton> gamepad_button(std::uint8_t button) noexcept;
    [[nodiscard]] static float axis_value(GamepadAxis axis, std::int16_t value) noexcept;
    [[nodiscard]] static Key key(std::int32_t scancode) noexcept;
    [[nodiscard]] static std::optional<MouseButton> mouse_button(std::uint8_t button) noexcept;

    SDL_Gamepad* gamepad = nullptr;
    std::uint32_t gamepadId = 0;
};

}
