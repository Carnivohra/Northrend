#pragma once

#include "Math/quaternion.hpp"
#include "Math/vector.hpp"
#include "Render/camera.hpp"

#include <cstdint>

namespace Northrend {

struct Camera final {
    Vector3 position{ 0.0F, 0.0F, 5.0F };
    Quaternion orientation;
    float verticalFieldOfView = 1.0471975512F;
    float nearPlane = 0.1F;
    float farPlane = 10'000.0F;
    std::uint64_t revision = 1;
};

[[nodiscard]] RenderCamera extract_camera(const Camera& camera) noexcept;

}
