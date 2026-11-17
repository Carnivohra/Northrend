#pragma once

#include "Math/vector.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class AssetData;

class WmoGroup final {
public:
    struct Header final {
        std::uint32_t nameOffset;
        std::uint32_t descriptiveNameOffset;
        std::uint32_t flags;
        Vector3 minimum;
        Vector3 maximum;
        std::uint16_t portalStart;
        std::uint16_t portalCount;
        std::uint16_t transparentBatchCount;
        std::uint16_t interiorBatchCount;
        std::uint16_t exteriorBatchCount;
        std::uint16_t additionalBatchCount;
        std::array<std::uint8_t, 4> fogIds;
        std::uint32_t liquidType;
        std::uint32_t id;
    };

    struct Polygon final {
        std::uint8_t flags;
        std::uint8_t material;
    };

    struct TextureCoordinate final {
        float u;
        float v;
    };

    struct Batch final {
        std::array<std::int16_t, 3> minimum;
        std::array<std::int16_t, 3> maximum;
        std::uint32_t firstIndex;
        std::uint16_t indexCount;
        std::uint16_t firstVertex;
        std::uint16_t lastVertex;
        std::uint8_t flags;
        std::uint8_t material;
    };

    explicit WmoGroup(std::shared_ptr<const AssetData> data);

    [[nodiscard]] std::uint32_t version() const noexcept;
    [[nodiscard]] const Header& header() const noexcept;
    [[nodiscard]] std::span<const Polygon> polygons() const noexcept;
    [[nodiscard]] std::span<const std::uint16_t> indices() const noexcept;
    [[nodiscard]] std::span<const Vector3> vertices() const noexcept;
    [[nodiscard]] std::span<const Vector3> normals() const noexcept;
    [[nodiscard]] std::size_t texture_coordinate_set_count() const noexcept;
    [[nodiscard]] std::span<const TextureCoordinate> texture_coordinates(std::size_t set) const;
    [[nodiscard]] std::span<const Batch> batches() const noexcept;

private:
    void parse_version17();
    [[nodiscard]] static std::int16_t read_int16(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static std::uint16_t read_uint16(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static std::uint32_t read_uint32(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static float read_float(std::span<const std::byte> bytes, std::size_t offset);
    [[nodiscard]] static Vector3 read_vector(std::span<const std::byte> bytes, std::size_t offset);

    std::shared_ptr<const AssetData> data;
    Header groupHeader {};
    std::vector<Polygon> polygonData;
    std::vector<std::uint16_t> indexData;
    std::vector<Vector3> vertexData;
    std::vector<Vector3> normalData;
    std::vector<std::vector<TextureCoordinate>> textureCoordinateData;
    std::vector<Batch> batchData;
    std::uint32_t fileVersion = 0;
};

}
