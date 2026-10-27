#pragma once

#include "Render/camera.hpp"
#include "Render/instance.hpp"

#include <cstdint>
#include <span>

namespace Northrend {

struct RenderFrame final {
    std::span<const RenderInstance> instances;
    std::uint64_t instancesRevision;
    const Camera& camera;
    const Matrix4& view;
    std::uint64_t cameraRevision;
};

}
