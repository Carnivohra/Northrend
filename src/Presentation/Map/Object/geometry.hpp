#pragma once

#include "Math/vector.hpp"
#include "Render/Mesh/vertex.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Northrend {

class MapObjectAsset;

struct PresentationMapObjectSection final {
    std::uint32_t firstIndex;
    std::uint32_t indexCount;
    std::uint32_t material;
    std::uint32_t group;
};

struct PresentationMapObjectGroup final {
    Vector3 minimum;
    Vector3 maximum;
};

class PresentationMapObjectGeometry final {
public:
    explicit PresentationMapObjectGeometry(const MapObjectAsset& object);

    [[nodiscard]] std::span<const MeshVertex> vertices() const noexcept;
    [[nodiscard]] std::span<const std::uint32_t> indices() const noexcept;
    [[nodiscard]] std::span<const PresentationMapObjectSection> sections() const noexcept;
    [[nodiscard]] std::span<const PresentationMapObjectGroup> groups() const noexcept;

private:
    [[nodiscard]] static std::int8_t encode_normal(float value) noexcept;

    std::vector<MeshVertex> meshVertices;
    std::vector<std::uint32_t> meshIndices;
    std::vector<PresentationMapObjectSection> meshSections;
    std::vector<PresentationMapObjectGroup> meshGroups;
};

}
