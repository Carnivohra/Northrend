#pragma once

#include <cstddef>
#include <optional>
#include <span>

namespace Northrend {

enum class DbcFormat { wdbc };

[[nodiscard]] std::optional<DbcFormat> dbc_format(std::span<const std::byte> bytes) noexcept;

}
