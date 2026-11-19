#include "Backend/SDL3/input.hpp"

#include "Input/input.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

#include <algorithm>

namespace Northrend {

void SDL3Input::process(const SDL_Event& event, Input& input) noexcept {
    switch (event.type) {
    case SDL_EVENT_GAMEPAD_ADDED:
        this->open(event.gdevice.which, input);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        this->remove(event.gdevice.which, input);
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (this->active(event.gaxis.which)) {
            if (const std::optional<GamepadAxis> axis = gamepad_axis(event.gaxis.axis)) {
                input.gamepad().move(*axis, axis_value(*axis, event.gaxis.value));
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (this->active(event.gbutton.which)) {
            if (const std::optional<GamepadButton> button = gamepad_button(event.gbutton.button)) {
                if (event.gbutton.down) {
                    input.gamepad().press(*button);
                } else {
                    input.gamepad().release(*button);
                }
            }
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        input.keyboard().press(key(event.key.scancode));
        break;
    case SDL_EVENT_KEY_UP:
        input.keyboard().release(key(event.key.scancode));
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (const std::optional<MouseButton> button = mouse_button(event.button.button)) {
            input.mouse().press(*button);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (const std::optional<MouseButton> button = mouse_button(event.button.button)) {
            input.mouse().release(*button);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        input.mouse().move(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
        break;
    case SDL_EVENT_MOUSE_WHEEL: {
        const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0F : 1.0F;
        input.mouse().scroll(event.wheel.x * direction, event.wheel.y * direction);
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        input.neutralize();
        break;
    default:
        break;
    }
}

void SDL3Input::close() noexcept {
    if (this->gamepad) {
        SDL_CloseGamepad(this->gamepad);
        this->gamepad = nullptr;
        this->gamepadId = 0;
    }
}

void SDL3Input::open(const std::uint32_t id, Input& input) noexcept {
    if (this->gamepad) {
        return;
    }

    SDL_Gamepad* opened = SDL_OpenGamepad(id);

    if (!opened) {
        return;
    }

    this->gamepad = opened;
    this->gamepadId = SDL_GetGamepadID(opened);
    input.gamepad().connect();
}

void SDL3Input::open_available(Input& input) noexcept {
    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);

    if (gamepads && count > 0) {
        this->open(gamepads[0], input);
    }

    SDL_free(gamepads);
}

void SDL3Input::remove(const std::uint32_t id, Input& input) noexcept {
    if (!this->active(id)) {
        return;
    }

    this->close();
    input.gamepad().disconnect();
    this->open_available(input);
}

bool SDL3Input::active(const std::uint32_t id) const noexcept {
    return this->gamepad && id == this->gamepadId;
}

std::optional<GamepadAxis> SDL3Input::gamepad_axis(const std::uint8_t axis) noexcept {
    switch (static_cast<SDL_GamepadAxis>(axis)) {
    case SDL_GAMEPAD_AXIS_LEFTX:
        return GamepadAxis::leftX;
    case SDL_GAMEPAD_AXIS_LEFTY:
        return GamepadAxis::leftY;
    case SDL_GAMEPAD_AXIS_RIGHTX:
        return GamepadAxis::rightX;
    case SDL_GAMEPAD_AXIS_RIGHTY:
        return GamepadAxis::rightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        return GamepadAxis::leftTrigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        return GamepadAxis::rightTrigger;
    default:
        return std::nullopt;
    }
}

std::optional<GamepadButton> SDL3Input::gamepad_button(const std::uint8_t button) noexcept {
    switch (static_cast<SDL_GamepadButton>(button)) {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return GamepadButton::south;
    case SDL_GAMEPAD_BUTTON_EAST:
        return GamepadButton::east;
    case SDL_GAMEPAD_BUTTON_WEST:
        return GamepadButton::west;
    case SDL_GAMEPAD_BUTTON_NORTH:
        return GamepadButton::north;
    case SDL_GAMEPAD_BUTTON_BACK:
        return GamepadButton::back;
    case SDL_GAMEPAD_BUTTON_GUIDE:
        return GamepadButton::guide;
    case SDL_GAMEPAD_BUTTON_START:
        return GamepadButton::start;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return GamepadButton::leftStick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return GamepadButton::rightStick;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return GamepadButton::leftShoulder;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return GamepadButton::rightShoulder;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return GamepadButton::up;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return GamepadButton::down;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return GamepadButton::left;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return GamepadButton::right;
    default:
        return std::nullopt;
    }
}

float SDL3Input::axis_value(const GamepadAxis axis, const std::int16_t value) noexcept {
    if (axis == GamepadAxis::leftTrigger || axis == GamepadAxis::rightTrigger) {
        return static_cast<float>(std::max<std::int16_t>(value, 0)) / 32767.0F;
    }

    return value < 0 ? static_cast<float>(value) / 32768.0F : static_cast<float>(value) / 32767.0F;
}

Key SDL3Input::key(const std::int32_t scancode) noexcept {
    const auto value = static_cast<std::uint16_t>(scancode);

    if (scancode == SDL_SCANCODE_UNKNOWN || value >= static_cast<std::uint16_t>(Key::count)) {
        return Key::unknown;
    }

    return static_cast<Key>(value);
}

std::optional<MouseButton> SDL3Input::mouse_button(const std::uint8_t button) noexcept {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return MouseButton::left;
    case SDL_BUTTON_MIDDLE:
        return MouseButton::middle;
    case SDL_BUTTON_RIGHT:
        return MouseButton::right;
    case SDL_BUTTON_X1:
        return MouseButton::back;
    case SDL_BUTTON_X2:
        return MouseButton::forward;
    default:
        return std::nullopt;
    }
}

}
