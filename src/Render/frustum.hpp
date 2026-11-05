#pragma once

#include "Math/matrix.hpp"
#include "Math/vector.hpp"

#include <array>
#include <cstddef>

namespace Northrend {

class Frustum final {
public:
    explicit Frustum(const Matrix4& matrix) noexcept;

    [[nodiscard]] bool intersects(const Vector3& minimum, const Vector3& maximum) const noexcept;

private:
    static constexpr std::size_t PlaneCount = 6;
    static constexpr std::size_t PlaneSize = 4;

    std::array<std::array<float, PlaneSize>, PlaneCount> planes;
};

}
