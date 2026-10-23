#pragma once

#include "Math/matrix.hpp"
#include "Math/vector.hpp"

namespace Northrend {

struct RenderCamera final {
    Matrix4 view;
    Vector3 position;
    float verticalFieldOfView = 1.0471975512F;
    float nearPlane = 0.1F;
    float farPlane = 10'000.0F;
};

[[nodiscard]] Matrix4 projection_matrix(
    const RenderCamera& camera,
    float aspectRatio);

}
