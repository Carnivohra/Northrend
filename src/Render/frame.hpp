#pragma once

#include "Render/camera.hpp"
#include "Render/instance.hpp"
#include "Render/light.hpp"
#include "Render/sections.hpp"

#include <cstdint>
#include <span>

namespace Northrend {

struct RenderFrame final {
    std::span<const RenderInstance> instances;
    std::span<const RenderSections> sections;
    std::uint64_t instancesRevision;
    const Camera& camera;
    const Matrix4& view;
    std::uint64_t cameraRevision;
    const DirectionalLight& light;
};

}
