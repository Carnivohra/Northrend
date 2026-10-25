#include "Math/Glm/matrix.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace Northrend {

Matrix4 from_glm(const glm::mat4& source) noexcept {
    Matrix4 result;
    std::copy_n(
        glm::value_ptr(source),
        result.values.size(),
        result.values.begin());
    return result;
}

}
