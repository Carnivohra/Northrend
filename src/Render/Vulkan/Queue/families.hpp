#pragma once

#include <cstdint>
#include <optional>

namespace Northrend {

struct VulkanQueueFamilies final {
    std::optional<std::uint32_t> graphics;
    std::optional<std::uint32_t> present;

    [[nodiscard]] bool complete() const noexcept {
        return this->graphics.has_value() &&
            this->present.has_value();
    }
};

}
