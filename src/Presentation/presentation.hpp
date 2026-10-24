#pragma once

#include "ECS/registry.hpp"
#include "Render/camera.hpp"
#include "Render/frame.hpp"
#include "Render/snapshot.hpp"
#include "World/camera.hpp"

#include <cstdint>

namespace Northrend {

class Presentation final {
public:
    Presentation();

    void synchronize(const RenderSnapshot& snapshot) noexcept;

    [[nodiscard]] Registry& registry() noexcept;
    [[nodiscard]] const Registry& registry() const noexcept;
    [[nodiscard]] Camera& active_camera();
    [[nodiscard]] RenderFrame frame();

private:
    Registry state;
    Entity cameraEntity;
    RenderSnapshot latestSnapshot;
    RenderCamera cachedCamera;
    std::uint64_t cachedCameraRevision = 0;
};

}
