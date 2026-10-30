#pragma once

#include "Asset/wdt.hpp"

#include <cstddef>
#include <span>

namespace Northrend {

class MapLayout final {
public:
    explicit MapLayout(std::span<const std::byte> data);

    [[nodiscard]] bool has_tile(std::size_t x, std::size_t y) const noexcept;

private:
    Wdt mapDefinition;
};

}
