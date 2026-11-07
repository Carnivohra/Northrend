#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace Northrend {

class Timing final {
public:
    struct Summary final {
        double mean;
        double median;
        double percentile95;
        double percentile99;
        double maximum;
        std::uint64_t samples;
    };

    Timing();

    void reset() noexcept;
    [[nodiscard]] std::optional<Summary> record(double milliseconds);

    static void write(std::ostream& stream, std::string_view name, const Summary& summary);

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    [[nodiscard]] static double percentile(const std::vector<double>& values, double rank) noexcept;

    TimePoint interval;
    std::vector<double> values;
};

}
