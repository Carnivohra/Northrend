#include "Render/frustum.hpp"

#include <cmath>
#include <cstddef>

namespace Northrend {

Frustum::Frustum(const Matrix4& matrix) noexcept {
    const auto row = [&matrix](
                         const std::size_t row, const std::size_t column) { return matrix.values[column * 4 + row]; };
    const auto plane = [&row](const std::size_t first, const float factor, const std::size_t second) {
        std::array<float, PlaneSize> result;

        for (std::size_t component = 0; component < PlaneSize; ++component) {
            result[component] = row(first, component) + factor * row(second, component);
        }

        const float length = std::sqrt(result[0] * result[0] + result[1] * result[1] + result[2] * result[2]);

        if (length > 0.0F) {
            for (float& component : result) {
                component /= length;
            }
        }

        return result;
    };

    this->planes[0] = plane(3, 1.0F, 0);
    this->planes[1] = plane(3, -1.0F, 0);
    this->planes[2] = plane(3, 1.0F, 1);
    this->planes[3] = plane(3, -1.0F, 1);
    this->planes[4] = plane(2, 0.0F, 0);
    this->planes[5] = plane(3, -1.0F, 2);
}

bool Frustum::intersects(const Vector3& minimum, const Vector3& maximum) const noexcept {
    static constexpr float Margin = 1.0F;

    for (const auto& plane : this->planes) {
        const float x = plane[0] >= 0.0F ? maximum.x : minimum.x;
        const float y = plane[1] >= 0.0F ? maximum.y : minimum.y;
        const float z = plane[2] >= 0.0F ? maximum.z : minimum.z;

        if (plane[0] * x + plane[1] * y + plane[2] * z + plane[3] < -Margin) {
            return false;
        }
    }

    return true;
}

}
