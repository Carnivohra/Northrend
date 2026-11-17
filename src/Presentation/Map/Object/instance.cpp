#include "Presentation/Map/Object/instance.hpp"

#include "Math/Glm/matrix.hpp"
#include "World/Map/Object/placement.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <numbers>
#include <utility>

namespace Northrend {

PresentationMapObjectInstance::PresentationMapObjectInstance(
    std::shared_ptr<const PresentationMapObjectModel> model, const MapObjectPlacement& placement) noexcept :
    objectModel(std::move(model)),
    objectTransform(transform(placement)),
    minimumBounds(minimum(placement)),
    maximumBounds(maximum(placement)),
    placementId(placement.id) {}

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

std::uint32_t PresentationMapObjectInstance::id() const noexcept {
    return this->placementId;
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

}
