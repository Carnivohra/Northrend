#pragma once

#include "Render/camera.hpp"
#include "Render/snapshot.hpp"

#include <cstdint>

namespace Northrend {

struct RenderFrame final {
    const RenderSnapshot& snapshot;
    const RenderCamera& camera;
    std::uint64_t cameraRevision;
};

}
