#include "World/Camera/extraction.hpp"

#include "Math/Glm/matrix.hpp"
#include "World/camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Northrend {

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
        .view = from_glm(view),
        .position = camera.position,
        .verticalFieldOfView = camera.verticalFieldOfView,
        .nearPlane = camera.nearPlane,
        .farPlane = camera.farPlane
    };
}

}
