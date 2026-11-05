#pragma once

#include "Input/Keyboard/key.hpp"

#include <bitset>
#include <cstddef>

namespace Northrend {

class Keyboard final {
public:
    [[nodiscard]] bool down(Key key) const noexcept {
        return valid(key) && this->held.test(index(key));
    }

    [[nodiscard]] bool pressed(Key key) const noexcept {
        return valid(key) && this->pressedThisFrame.test(index(key));
    }

    [[nodiscard]] bool released(Key key) const noexcept {
        return valid(key) && this->releasedThisFrame.test(index(key));
    }

    void press(Key key) noexcept {
        if (!valid(key)) {
            return;
        }

        const std::size_t position = index(key);

        if (!this->held.test(position)) {
            this->held.set(position);
            this->pressedThisFrame.set(position);
        }
    }

    void release(Key key) noexcept {
        if (!valid(key)) {
            return;
        }

        const std::size_t position = index(key);

        if (this->held.test(position)) {
            this->held.reset(position);
            this->releasedThisFrame.set(position);
        }
    }

    void begin_frame() noexcept {
        this->pressedThisFrame.reset();
        this->releasedThisFrame.reset();
    }

    void reset() noexcept {
        this->releasedThisFrame |= this->held;
        this->held.reset();
        this->pressedThisFrame.reset();
    }

private:
    static constexpr std::size_t Count = static_cast<std::size_t>(Key::count);

    [[nodiscard]] static constexpr std::size_t index(Key key) noexcept {
        return static_cast<std::size_t>(key);
    }

    [[nodiscard]] static constexpr bool valid(Key key) noexcept {
        return key != Key::unknown && index(key) < Count;
    }

    std::bitset<Count> held;
    std::bitset<Count> pressedThisFrame;
    std::bitset<Count> releasedThisFrame;
};

}
