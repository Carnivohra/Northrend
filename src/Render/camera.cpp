#include "Render/camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <numbers>
#include <stdexcept>

namespace Northrend {
namespace {

Matrix4 to_matrix(const glm::mat4& source) noexcept {
    Matrix4 result;
    std::copy_n(glm::value_ptr(source), result.values.size(), result.values.begin());
    return result;
}

}

Matrix4 projection_matrix(
    const RenderCamera& camera,
    float aspectRatio) {
    if (aspectRatio <= 0.0F) {
        throw std::invalid_argument("Camera aspect ratio must be positive");
    }

    if (camera.verticalFieldOfView <= 0.0F ||
        camera.verticalFieldOfView >= std::numbers::pi_v<float>) {
        throw std::invalid_argument(
            "Camera field of view must be between zero and pi");
    }

    if (camera.nearPlane <= 0.0F || camera.farPlane <= camera.nearPlane) {
        throw std::invalid_argument(
            "Camera clipping planes must define a positive range");
    }

    return to_matrix(glm::perspectiveRH_ZO(
        camera.verticalFieldOfView,
        aspectRatio,
        camera.nearPlane,
        camera.farPlane));
}

}
