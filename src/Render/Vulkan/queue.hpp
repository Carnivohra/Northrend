#pragma once

#include <cstdint>
#include <optional>

namespace Northrend {

struct VulkanQueueFamilies final {
    std::optional<std::uint32_t> graphics;
    std::optional<std::uint32_t> present;
    std::optional<std::uint32_t> transfer;

    [[nodiscard]] bool complete() const noexcept {
        return this->graphics.has_value() && this->present.has_value() && this->transfer.has_value();
    }
};

}
