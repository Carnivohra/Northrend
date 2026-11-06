#pragma once

#include <cstddef>
#include <optional>

namespace Northrend {

class TileFocus final {
public:
    TileFocus() noexcept = default;
    TileFocus(std::size_t x, std::size_t y, std::size_t radius);

    [[nodiscard]] static TileFocus from_world(float x, float y, std::size_t radius) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] std::size_t x() const noexcept;
    [[nodiscard]] std::size_t y() const noexcept;
    [[nodiscard]] std::size_t radius() const noexcept;

    bool operator==(const TileFocus&) const = default;

private:
    static constexpr std::size_t Size = 64;

    std::size_t tileX = 0;
    std::size_t tileY = 0;
    std::size_t tileRadius = 0;
    bool focused = false;
};

}
