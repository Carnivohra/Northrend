#pragma once

#include <cstdint>

namespace Northrend {

class Map;

class MapSelection final {
public:
    [[nodiscard]] std::uint32_t id() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

private:
    friend class Map;

    MapSelection(std::uint32_t id, std::uint32_t generation) noexcept;

    std::uint32_t identifier;
    std::uint32_t selectionGeneration;
};

}
