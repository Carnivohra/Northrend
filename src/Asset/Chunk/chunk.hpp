#pragma once

#include "Asset/Chunk/id.hpp"

#include <cstddef>
#include <span>

namespace Northrend {

class Chunk final {
public:
    constexpr Chunk(ChunkId identifier, std::span<const std::byte> data) noexcept :
        identifier(identifier), data(data) {}

    [[nodiscard]] constexpr ChunkId id() const noexcept {
        return this->identifier;
    }

    [[nodiscard]] constexpr std::span<const std::byte> bytes() const noexcept {
        return this->data;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return this->data.size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return this->data.empty();
    }

private:
    ChunkId identifier;
    std::span<const std::byte> data;
};

}
