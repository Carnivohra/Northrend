#pragma once

#include "Render/camera.hpp"
#include "Render/instance.hpp"
#include "Render/snapshot.hpp"

#include <cstdint>
#include <span>

namespace Northrend {

struct RenderFrame final {
    const RenderSnapshot& snapshot;
    std::span<const RenderInstance> instances;
    std::uint64_t instancesRevision;
    const RenderCamera& camera;
    std::uint64_t cameraRevision;
};

}
