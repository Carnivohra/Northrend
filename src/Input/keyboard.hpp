#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace Northrend {

// Values follow the USB HID keyboard usage IDs used by SDL scancodes.
enum class Key : std::uint16_t {
    unknown = 0,
    a = 4,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,
    number1,
    number2,
    number3,
    number4,
    number5,
    number6,
    number7,
    number8,
    number9,
    number0,
    enter,
    escape,
    backspace,
    tab,
    space,
    minus,
    equals,
    leftBracket,
    rightBracket,
    backslash,
    nonUsHash,
    semicolon,
    apostrophe,
    grave,
    comma,
    period,
    slash,
    capsLock,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    printScreen,
    scrollLock,
    pause,
    insert,
    home,
    pageUp,
    deleteKey,
    end,
    pageDown,
    right,
    left,
    down,
    up,
    numLock,
    keypadDivide,
    keypadMultiply,
    keypadMinus,
    keypadPlus,
    keypadEnter,
    keypad1,
    keypad2,
    keypad3,
    keypad4,
    keypad5,
    keypad6,
    keypad7,
    keypad8,
    keypad9,
    keypad0,
    keypadPeriod,
    leftControl = 224,
    leftShift,
    leftAlt,
    leftSuper,
    rightControl,
    rightShift,
    rightAlt,
    rightSuper,
    count = 512
};

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
