#pragma once

#include "Math/matrix.hpp"

#include <glm/mat4x4.hpp>

namespace Northrend {

[[nodiscard]] Matrix4 from_glm(const glm::mat4& source) noexcept;

}
