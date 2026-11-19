#include "Presentation/Map/Object/instance.hpp"

#include "Math/Glm/matrix.hpp"
#include "Presentation/Map/Object/geometry.hpp"
#include "Presentation/Map/Object/model.hpp"
#include "World/Map/Object/placement.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace Northrend {

PresentationMapObjectInstance::PresentationMapObjectInstance(
    std::shared_ptr<const PresentationMapObjectModel> model, const MapObjectPlacement& placement) :
    objectModel(std::move(model)),
    objectTransform(transform(placement)),
    minimumBounds(minimum(placement)),
    maximumBounds(maximum(placement)) {
    const std::span<const PresentationMapObjectGroup> groups = this->objectModel->geometry().groups();
    this->groupMinimums.reserve(groups.size());
    this->groupMaximums.reserve(groups.size());

    for (const PresentationMapObjectGroup& group : groups) {
        const auto [minimum, maximum] = transform_bounds(this->objectTransform, group.minimum, group.maximum);
        this->groupMinimums.push_back(minimum);
        this->groupMaximums.push_back(maximum);
    }
}

const std::shared_ptr<const PresentationMapObjectModel>& PresentationMapObjectInstance::model() const noexcept {
    return this->objectModel;
}

const Matrix4& PresentationMapObjectInstance::transform() const noexcept {
    return this->objectTransform;
}

const Vector3& PresentationMapObjectInstance::minimum() const noexcept {
    return this->minimumBounds;
}

const Vector3& PresentationMapObjectInstance::maximum() const noexcept {
    return this->maximumBounds;
}

std::size_t PresentationMapObjectInstance::group_count() const noexcept {
    return this->groupMinimums.size();
}

const Vector3& PresentationMapObjectInstance::group_minimum(const std::size_t group) const noexcept {
    return this->groupMinimums[group];
}

const Vector3& PresentationMapObjectInstance::group_maximum(const std::size_t group) const noexcept {
    return this->groupMaximums[group];
}

Vector3 PresentationMapObjectInstance::position(const Vector3& source) noexcept {
    static constexpr float MapCenter = 17066.666F;
    return Vector3 {.x = MapCenter - source.z, .y = MapCenter - source.x, .z = source.y};
}

Vector3 PresentationMapObjectInstance::minimum(const MapObjectPlacement& placement) noexcept {
    static constexpr float MapCenter = 17066.666F;
    return Vector3 {
        .x = MapCenter - placement.maximum.z, .y = MapCenter - placement.maximum.x, .z = placement.minimum.y
    };
}

Vector3 PresentationMapObjectInstance::maximum(const MapObjectPlacement& placement) noexcept {
    static constexpr float MapCenter = 17066.666F;
    return Vector3 {
        .x = MapCenter - placement.minimum.z, .y = MapCenter - placement.minimum.x, .z = placement.maximum.y
    };
}

Matrix4 PresentationMapObjectInstance::transform(const MapObjectPlacement& placement) noexcept {
    const Vector3 translation = position(placement.position);
    const float radians = std::numbers::pi_v<float> / 180.0F;
    glm::mat4 result(1.0F);
    result = glm::translate(result, glm::vec3(translation.x, translation.y, translation.z));
    result = glm::rotate(result, placement.rotation.y * radians + std::numbers::pi_v<float>, glm::vec3(0, 0, 1));
    result = glm::rotate(result, placement.rotation.x * radians, glm::vec3(0, 1, 0));
    result = glm::rotate(result, placement.rotation.z * radians, glm::vec3(1, 0, 0));
    result = glm::scale(result, glm::vec3(placement.scale));
    return from_glm(result);
}

std::pair<Vector3, Vector3> PresentationMapObjectInstance::transform_bounds(
    const Matrix4& transform, const Vector3& minimum, const Vector3& maximum) noexcept {
    const Vector3 normalizedMinimum {
        .x = std::min(minimum.x, maximum.x),
        .y = std::min(minimum.y, maximum.y),
        .z = std::min(minimum.z, maximum.z)
    };
    const Vector3 normalizedMaximum {
        .x = std::max(minimum.x, maximum.x),
        .y = std::max(minimum.y, maximum.y),
        .z = std::max(minimum.z, maximum.z)
    };
    const Vector3 center {
        .x = (normalizedMinimum.x + normalizedMaximum.x) * 0.5F,
        .y = (normalizedMinimum.y + normalizedMaximum.y) * 0.5F,
        .z = (normalizedMinimum.z + normalizedMaximum.z) * 0.5F
    };
    const Vector3 extent {
        .x = (normalizedMaximum.x - normalizedMinimum.x) * 0.5F,
        .y = (normalizedMaximum.y - normalizedMinimum.y) * 0.5F,
        .z = (normalizedMaximum.z - normalizedMinimum.z) * 0.5F
    };
    const Vector3 transformedCenter {
        .x = transform.values[0] * center.x + transform.values[4] * center.y + transform.values[8] * center.z +
            transform.values[12],
        .y = transform.values[1] * center.x + transform.values[5] * center.y + transform.values[9] * center.z +
            transform.values[13],
        .z = transform.values[2] * center.x + transform.values[6] * center.y + transform.values[10] * center.z +
            transform.values[14]
    };
    const Vector3 transformedExtent {
        .x = std::abs(transform.values[0]) * extent.x + std::abs(transform.values[4]) * extent.y +
            std::abs(transform.values[8]) * extent.z,
        .y = std::abs(transform.values[1]) * extent.x + std::abs(transform.values[5]) * extent.y +
            std::abs(transform.values[9]) * extent.z,
        .z = std::abs(transform.values[2]) * extent.x + std::abs(transform.values[6]) * extent.y +
            std::abs(transform.values[10]) * extent.z
    };

    return {
        Vector3 {
            .x = transformedCenter.x - transformedExtent.x,
            .y = transformedCenter.y - transformedExtent.y,
            .z = transformedCenter.z - transformedExtent.z
        },
        Vector3 {
            .x = transformedCenter.x + transformedExtent.x,
            .y = transformedCenter.y + transformedExtent.y,
            .z = transformedCenter.z + transformedExtent.z
        }
    };
}

}
