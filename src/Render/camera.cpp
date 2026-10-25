#include "Render/camera.hpp"

#include "Math/Glm/matrix.hpp"

#include <glm/ext/matrix_clip_space.hpp>

#include <numbers>
#include <stdexcept>

namespace Northrend {

Matrix4 RenderCamera::projection(float aspectRatio) const {
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
