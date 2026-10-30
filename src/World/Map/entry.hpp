#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Northrend {

class MapEntry final {
public:
    MapEntry(std::uint32_t id, std::string directory);

    [[nodiscard]] std::uint32_t id() const noexcept;
    [[nodiscard]] std::string_view directory() const noexcept;

private:
    std::uint32_t identifier;
    std::string directoryName;
};

}
