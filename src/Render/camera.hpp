#pragma once

#include "Math/matrix.hpp"
#include "Math/quaternion.hpp"
#include "Math/vector.hpp"

#include <cstdint>

namespace Northrend {

struct Camera final {
    [[nodiscard]] Matrix4 view() const noexcept;
    [[nodiscard]] Matrix4 projection(float aspectRatio) const;
    [[nodiscard]] Vector3 forward() const noexcept;
    [[nodiscard]] Vector3 right() const noexcept;
    [[nodiscard]] Vector3 up() const noexcept;
    void look_at(Vector3 target, Vector3 up = Vector3 {.z = 1.0F});

    Vector3 position {0.0F, 0.0F, 5.0F};
    Quaternion orientation;
    float verticalFieldOfView = 1.0471975512F;
    float nearPlane = 0.1F;
    float farPlane = 10'000.0F;
    std::uint64_t revision = 1;

private:
    [[nodiscard]] Vector3 rotate(Vector3 direction) const noexcept;
};

}
