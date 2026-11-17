#pragma once

#include "Asset/Format/wdt.hpp"
#include "Math/vector.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace Northrend {

class MapLayout final {
public:
    explicit MapLayout(std::span<const std::byte> data);

    [[nodiscard]] bool has_tile(std::size_t x, std::size_t y) const noexcept;
    [[nodiscard]] bool high_resolution_alpha() const noexcept;
    [[nodiscard]] const std::optional<Vector3>& center() const noexcept;

private:
    Wdt mapDefinition;
    std::optional<Vector3> mapCenter;
};

}
