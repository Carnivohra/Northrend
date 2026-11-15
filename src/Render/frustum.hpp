#pragma once

#include "Math/matrix.hpp"
#include "Math/vector.hpp"

#include <array>
#include <cstddef>

namespace Northrend {

class Frustum final {
public:
    enum class Relation {
        outside,
        intersecting,
        inside
    };

    explicit Frustum(const Matrix4& matrix) noexcept;

    [[nodiscard]] Relation classify(const Vector3& minimum, const Vector3& maximum) const noexcept;
    [[nodiscard]] bool intersects(const Vector3& minimum, const Vector3& maximum) const noexcept;

private:
    static constexpr std::size_t PlaneCount = 6;
    static constexpr std::size_t PlaneSize = 4;
    static constexpr float Margin = 1.0F;

    std::array<std::array<float, PlaneSize>, PlaneCount> planes;
};

}
