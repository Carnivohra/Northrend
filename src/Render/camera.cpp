#include "Render/camera.hpp"

#include "Math/Glm/matrix.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <numbers>
#include <stdexcept>

namespace Northrend {

Matrix4 Camera::view() const noexcept {
    glm::quat orientation(
        this->orientation.w,
        this->orientation.x,
        this->orientation.y,
        this->orientation.z);

    const float magnitude = glm::dot(orientation, orientation);

    if (magnitude > 0.0F) {
        orientation *= glm::inversesqrt(magnitude);
    } else {
        orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    }

    const glm::vec3 position(
        this->position.x,
        this->position.y,
        this->position.z);
    const glm::mat4 view =
        glm::mat4_cast(glm::conjugate(orientation)) *
        glm::translate(glm::mat4(1.0F), -position);

    return from_glm(view);
}

Matrix4 Camera::projection(float aspectRatio) const {
    if (aspectRatio <= 0.0F) {
        throw std::invalid_argument("Camera aspect ratio must be positive");
    }

    if (this->verticalFieldOfView <= 0.0F ||
        this->verticalFieldOfView >= std::numbers::pi_v<float>) {
        throw std::invalid_argument(
            "Camera field of view must be between zero and pi");
    }

    if (this->nearPlane <= 0.0F ||
        this->farPlane <= this->nearPlane) {
        throw std::invalid_argument(
            "Camera clipping planes must define a positive range");
    }

    return from_glm(glm::perspectiveRH_ZO(
        this->verticalFieldOfView,
        aspectRatio,
        this->nearPlane,
        this->farPlane));
}

}
