#include "Asset/Dbc/format.hpp"

namespace Northrend {

std::optional<DbcFormat> dbc_format(const std::span<const std::byte> bytes) noexcept {
    if (bytes.size() >= 4 && bytes[0] == std::byte {'W'} && bytes[1] == std::byte {'D'} &&
        bytes[2] == std::byte {'B'} && bytes[3] == std::byte {'C'}) {
        return DbcFormat::wdbc;
    }

    return std::nullopt;
}

}
