#include "World/camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace Northrend {
namespace {

Matrix4 to_matrix(const glm::mat4& source) noexcept {
    Matrix4 result;
    std::copy_n(glm::value_ptr(source), result.values.size(), result.values.begin());
    return result;
}

}

RenderCamera extract_camera(const Camera& camera) noexcept {
    glm::quat orientation(
        camera.orientation.w,
        camera.orientation.x,
        camera.orientation.y,
        camera.orientation.z);

    const float magnitude = glm::dot(orientation, orientation);

    if (magnitude > 0.0F) {
        orientation *= glm::inversesqrt(magnitude);
    } else {
        orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    }

    const glm::vec3 position(
        camera.position.x,
        camera.position.y,
        camera.position.z);
    const glm::mat4 view =
        glm::mat4_cast(glm::conjugate(orientation)) *
        glm::translate(glm::mat4(1.0F), -position);

    return RenderCamera{
        .view = to_matrix(view),
        .position = camera.position,
        .verticalFieldOfView = camera.verticalFieldOfView,
        .nearPlane = camera.nearPlane,
        .farPlane = camera.farPlane
    };
}

}
