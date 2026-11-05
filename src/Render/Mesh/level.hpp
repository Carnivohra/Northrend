#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Northrend {

enum class MeshIndexType : std::uint8_t { unsigned16, unsigned32 };

struct MeshLevel final {
    std::span<const std::byte> indices;
    std::uint32_t indexCount;
};

}
