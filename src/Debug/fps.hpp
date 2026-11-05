#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace Northrend {

class Fps final {
public:
    Fps() noexcept {
        this->reset();
    }

    void reset() noexcept {
        this->started = Clock::now();
        this->frames = 0;
    }

    std::optional<double> record_frame() noexcept {
        ++this->frames;

        const TimePoint now = Clock::now();
        const std::chrono::duration<double> elapsed = now - this->started;

        if (elapsed < std::chrono::seconds(1)) {
            return std::nullopt;
        }

        const double framesPerSecond = static_cast<double>(this->frames) / elapsed.count();
        this->started = now;
        this->frames = 0;

        return framesPerSecond;
    }

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimePoint started;
    std::uint64_t frames = 0;
};

}
