#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Northrend {

enum class MeshIndexType : std::uint8_t { unsigned16, unsigned32 };

class MeshData final {
public:
    MeshData(std::span<const std::byte> vertices, std::span<const std::byte> indices, std::uint32_t indexCount,
        MeshIndexType indexType);

    [[nodiscard]] std::span<const std::byte> vertices() const noexcept;
    [[nodiscard]] std::span<const std::byte> indices() const noexcept;
    [[nodiscard]] std::uint32_t index_count() const noexcept;
    [[nodiscard]] MeshIndexType index_type() const noexcept;

private:
    std::span<const std::byte> vertexData;
    std::span<const std::byte> indexData;
    std::uint32_t indexCount;
    MeshIndexType indexType;
};

}
