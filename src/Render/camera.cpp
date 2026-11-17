#include "Render/camera.hpp"

#include "Math/Glm/matrix.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <numbers>
#include <stdexcept>

namespace Northrend {

Matrix4 Camera::view() const noexcept {
    glm::quat orientation(this->orientation.w, this->orientation.x, this->orientation.y, this->orientation.z);
    const float magnitude = glm::dot(orientation, orientation);

    if (magnitude > 0.0F) {
        orientation *= glm::inversesqrt(magnitude);
    } else {
        orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    }

    const glm::vec3 position(this->position.x, this->position.y, this->position.z);
    glm::mat4 axes(1.0F);
    axes[0] = glm::vec4(0.0F, 0.0F, -1.0F, 0.0F);
    axes[1] = glm::vec4(-1.0F, 0.0F, 0.0F, 0.0F);
    axes[2] = glm::vec4(0.0F, 1.0F, 0.0F, 0.0F);
    const glm::mat4 view =
        axes * glm::mat4_cast(glm::conjugate(orientation)) * glm::translate(glm::mat4(1.0F), -position);

    return from_glm(view);
}

Matrix4 Camera::projection(float aspectRatio) const {
    if (aspectRatio <= 0.0F) {
        throw std::invalid_argument("Camera aspect ratio must be positive");
    }

    if (this->verticalFieldOfView <= 0.0F || this->verticalFieldOfView >= std::numbers::pi_v<float>) {
        throw std::invalid_argument("Camera field of view must be between zero and pi");
    }

    if (this->nearPlane <= 0.0F || this->farPlane <= this->nearPlane) {
        throw std::invalid_argument("Camera clipping planes must define a positive range");
    }

    return from_glm(glm::perspectiveRH_ZO(this->verticalFieldOfView, aspectRatio, this->farPlane, this->nearPlane));
}

Vector3 Camera::forward() const noexcept {
    return this->rotate(Vector3 {.x = 1.0F});
}

Vector3 Camera::right() const noexcept {
    return this->rotate(Vector3 {.y = -1.0F});
}

Vector3 Camera::up() const noexcept {
    return this->rotate(Vector3 {.z = 1.0F});
}

void Camera::look_at(const Vector3 target, const Vector3 up) {
    this->look_to(Vector3 {
        .x = target.x - this->position.x,
        .y = target.y - this->position.y,
        .z = target.z - this->position.z
    }, up);
}

void Camera::look_to(const Vector3 direction, const Vector3 up) {
    const glm::vec3 forwardDirection(direction.x, direction.y, direction.z);
    const glm::vec3 upward(up.x, up.y, up.z);

    if (glm::dot(forwardDirection, forwardDirection) <= 0.0F || glm::dot(upward, upward) <= 0.0F ||
        glm::dot(glm::cross(forwardDirection, upward), glm::cross(forwardDirection, upward)) <= 0.0F) {
        throw std::invalid_argument("Camera direction and up vector must define a valid orientation");
    }

    const glm::vec3 forward = glm::normalize(forwardDirection);
    const glm::vec3 right = glm::normalize(glm::cross(forward, upward));
    const glm::vec3 correctedUp = glm::normalize(glm::cross(right, forward));
    glm::mat3 basis(1.0F);
    basis[0] = forward;
    basis[1] = -right;
    basis[2] = correctedUp;

    const glm::quat orientation = glm::quat_cast(basis);
    this->orientation = Quaternion {.x = orientation.x, .y = orientation.y, .z = orientation.z, .w = orientation.w};
}

Vector3 Camera::rotate(const Vector3 direction) const noexcept {
    glm::quat orientation(this->orientation.w, this->orientation.x, this->orientation.y, this->orientation.z);
    const float magnitude = glm::dot(orientation, orientation);

    if (magnitude > 0.0F) {
        orientation *= glm::inversesqrt(magnitude);
    } else {
        orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    }

    const glm::vec3 rotated = orientation * glm::vec3(direction.x, direction.y, direction.z);
    return Vector3 {.x = rotated.x, .y = rotated.y, .z = rotated.z};
}

}
