#include "Debug/timing.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace Northrend {

Timing::Timing() {
    this->values.reserve(65536);
    this->reset();
}

void Timing::reset() noexcept {
    this->interval = Clock::now();
    this->values.clear();
}

std::optional<Timing::Summary> Timing::record(const double milliseconds) {
    if (!std::isfinite(milliseconds) || milliseconds < 0.0) {
        throw std::invalid_argument("Timing sample must be a finite non-negative duration");
    }

    this->values.push_back(milliseconds);
    const TimePoint now = Clock::now();

    if (now - this->interval < std::chrono::seconds(1)) {
        return std::nullopt;
    }

    std::vector<double> sorted = this->values;
    std::ranges::sort(sorted);
    const double total = std::accumulate(sorted.begin(), sorted.end(), 0.0);
    const Summary summary {
        .mean = total / static_cast<double>(sorted.size()),
        .median = percentile(sorted, 0.50),
        .percentile95 = percentile(sorted, 0.95),
        .percentile99 = percentile(sorted, 0.99),
        .maximum = sorted.back(),
        .samples = static_cast<std::uint64_t>(sorted.size())
    };
    this->interval = now;
    this->values.clear();
    return summary;
}

void Timing::write(std::ostream& stream, const std::string_view name, const Summary& summary) {
    const double medianFps = summary.median > std::numeric_limits<double>::epsilon()
        ? 1000.0 / summary.median
        : std::numeric_limits<double>::infinity();
    const std::ios::fmtflags flags = stream.flags();
    const std::streamsize precision = stream.precision();
    stream << std::fixed << std::setprecision(3) << name << " ms: mean=" << summary.mean
           << " p50=" << summary.median << " p95=" << summary.percentile95 << " p99=" << summary.percentile99
           << " max=" << summary.maximum << " fps=" << std::setprecision(0) << medianFps
           << " samples=" << summary.samples << '\n';
    stream.flags(flags);
    stream.precision(precision);
}

double Timing::percentile(const std::vector<double>& values, const double rank) noexcept {
    const double position = std::ceil(rank * static_cast<double>(values.size()));
    const std::size_t index = static_cast<std::size_t>(std::max(position, 1.0)) - 1;
    return values[std::min(index, values.size() - 1)];
}

}
