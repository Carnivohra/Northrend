#include "Presentation/Map/Object/geometry.hpp"

#include "Asset/Format/Wmo/group.hpp"
#include "World/Map/Object/asset.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Northrend {

std::int8_t PresentationMapObjectGeometry::encode_normal(const float value) noexcept {
    const float normalized = std::clamp(value, -1.0F, 1.0F);
    return static_cast<std::int8_t>(std::lround(normalized * 127.0F));
}

PresentationMapObjectGeometry::PresentationMapObjectGeometry(const MapObjectAsset& object) :
    minimumBounds(object.root().header().minimum), maximumBounds(object.root().header().maximum) {
    const std::span<const WmoGroup> groups = object.groups();
    std::size_t vertexCount = 0;
    std::size_t indexCount = 0;
    std::size_t sectionCount = 0;

    for (const WmoGroup& group : groups) {
        vertexCount += group.vertices().size();
        indexCount += group.indices().size();
        sectionCount += group.batches().size();
    }

    if (vertexCount > std::numeric_limits<std::uint32_t>::max() ||
        indexCount > std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("WMO geometry exceeds the presentation index range");
    }

    this->meshVertices.reserve(vertexCount);
    this->meshIndices.reserve(indexCount);
    this->meshSections.reserve(sectionCount);
    this->meshGroups.reserve(groups.size());

    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const WmoGroup& group = groups[groupIndex];
        const std::span<const Vector3> positions = group.vertices();
        const std::span<const Vector3> normals = group.normals();
        const std::span<const WmoGroup::TextureCoordinate> coordinates = group.texture_coordinates(0);
        const std::uint32_t vertexOffset = static_cast<std::uint32_t>(this->meshVertices.size());
        const std::uint32_t indexOffset = static_cast<std::uint32_t>(this->meshIndices.size());
        const std::uint32_t sectionOffset = static_cast<std::uint32_t>(this->meshSections.size());

        for (std::size_t vertex = 0; vertex < positions.size(); ++vertex) {
            const Vector3& normal = normals[vertex];
            this->meshVertices.push_back(MeshVertex {
                .position = positions[vertex],
                .normal = {encode_normal(normal.x), encode_normal(normal.y), encode_normal(normal.z), 0},
                .textureCoordinates = {coordinates[vertex].u, coordinates[vertex].v}
            });
        }

        for (const std::uint16_t index : group.indices()) {
            this->meshIndices.push_back(vertexOffset + index);
        }

        for (const WmoGroup::Batch& batch : group.batches()) {
            this->meshSections.push_back(PresentationMapObjectSection {
                .firstIndex = indexOffset + batch.firstIndex,
                .indexCount = batch.indexCount,
                .material = batch.material,
                .group = static_cast<std::uint32_t>(groupIndex)
            });
        }

        this->meshGroups.push_back(PresentationMapObjectGroup {
            .minimum = group.header().minimum,
            .maximum = group.header().maximum,
            .firstSection = sectionOffset,
            .sectionCount = static_cast<std::uint32_t>(group.batches().size()),
            .flags = group.header().flags
        });
    }
}

std::span<const MeshVertex> PresentationMapObjectGeometry::vertices() const noexcept {
    return this->meshVertices;
}

std::span<const std::uint32_t> PresentationMapObjectGeometry::indices() const noexcept {
    return this->meshIndices;
}

std::span<const PresentationMapObjectSection> PresentationMapObjectGeometry::sections() const noexcept {
    return this->meshSections;
}

std::span<const PresentationMapObjectGroup> PresentationMapObjectGeometry::groups() const noexcept {
    return this->meshGroups;
}

const Vector3& PresentationMapObjectGeometry::minimum() const noexcept {
    return this->minimumBounds;
}

const Vector3& PresentationMapObjectGeometry::maximum() const noexcept {
    return this->maximumBounds;
}

}
