#pragma once

#include "Asset/Chunk/chunk.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace Northrend {

class ChunkReader final {
public:
    explicit ChunkReader(std::span<const std::byte> bytes) noexcept;

    [[nodiscard]] std::optional<Chunk> next();
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t position() const noexcept;

private:
    std::span<const std::byte> data;
    std::size_t offset = 0;
};

}
