#pragma once

#include "Render/camera.hpp"

namespace Northrend {

struct Camera;

[[nodiscard]] RenderCamera extract_camera(const Camera& camera) noexcept;

}
