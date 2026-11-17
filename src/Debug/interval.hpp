#pragma once

#include <chrono>

namespace Northrend {

class Interval final {
public:
    [[nodiscard]] bool elapsed() noexcept {
        const TimePoint now = Clock::now();

        if (now - this->start < std::chrono::seconds(1)) {
            return false;
        }

        this->start = now;
        return true;
    }

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimePoint start = Clock::now();
};

}
