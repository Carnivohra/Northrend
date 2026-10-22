#pragma once

#include <compare>
#include <cstdint>

namespace Northrend {

class Registry;

class Entity final {
public:
    constexpr Entity() noexcept = default;

    [[nodiscard]] constexpr std::uint64_t id() const noexcept {
        return this->value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return this->value != 0;
    }

    auto operator<=>(const Entity&) const = default;

private:
    friend Registry;

    explicit constexpr Entity(std::uint64_t value) noexcept
        : value(value) {}

    std::uint64_t value = 0;
};

}
