#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <stdexcept>

namespace Northrend {

class TickRate final {
public:
    [[nodiscard]] static TickRate fixed(std::uint32_t ticksPerSecond) {
        if (ticksPerSecond == 0 || ticksPerSecond > 1'000'000'000) {
            throw std::invalid_argument(
                "Fixed tick rate must be between 1 and 1000000000");
        }

        return TickRate(ticksPerSecond);
    }

    [[nodiscard]] static constexpr TickRate unlimited() noexcept {
        return TickRate(0);
    }

    [[nodiscard]] constexpr bool is_unlimited() const noexcept {
        return this->ticksPerSecond == 0;
    }

    [[nodiscard]] constexpr std::uint32_t value() const noexcept {
        return this->ticksPerSecond;
    }

    [[nodiscard]] constexpr std::chrono::nanoseconds interval() const noexcept {
        if (this->is_unlimited()) {
            return std::chrono::nanoseconds::zero();
        }

        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::seconds(1)) / this->ticksPerSecond;
    }

    auto operator<=>(const TickRate&) const = default;

private:
    explicit constexpr TickRate(std::uint32_t ticksPerSecond) noexcept
        : ticksPerSecond(ticksPerSecond) {}

    std::uint32_t ticksPerSecond;
};

}
