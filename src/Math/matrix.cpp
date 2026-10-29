#include "Math/matrix.hpp"

#include <cstddef>

namespace Northrend {

Matrix4 Matrix4::operator*(const Matrix4& right) const noexcept {
    Matrix4 result;

    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            float value = 0.0F;

            for (std::size_t index = 0; index < 4; ++index) {
                value += this->values[index * 4 + row] * right.values[column * 4 + index];
            }

            result.values[column * 4 + row] = value;
        }
    }

    return result;
}

}
