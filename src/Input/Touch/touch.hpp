#pragma once

#include "Input/Touch/point.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace Northrend {

class Touch final {
public:
    [[nodiscard]] std::span<const TouchPoint> points() const noexcept {
        return this->activePoints;
    }

    void reset() noexcept {
        this->touchPoints.fill({});
        this->activePoints = {};
    }

private:
    static constexpr std::size_t MaximumPoints = 10;

    std::array<TouchPoint, MaximumPoints> touchPoints {};
    std::span<const TouchPoint> activePoints;
};

}
