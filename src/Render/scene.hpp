#pragma once

#include "ECS/registry.hpp"
#include "Render/camera.hpp"
#include "Render/frame.hpp"
#include "Render/instance.hpp"

#include <concepts>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Northrend {

class RenderScene final {
public:
    RenderScene();

    [[nodiscard]] Registry& registry() noexcept;
    [[nodiscard]] const Registry& registry() const noexcept;
    [[nodiscard]] const Camera& active_camera() const;

    template <typename Callback>
        requires std::invocable<Callback, Camera&>
    void modify_active_camera(Callback&& callback) {
        const bool modified = this->state.modify<Camera>(this->cameraEntity, [&callback](Camera& camera) {
            std::invoke(std::forward<Callback>(callback), camera);
            ++camera.revision;
        });

        if (!modified) {
            throw std::runtime_error("Render scene has no active camera");
        }
    }

    [[nodiscard]] RenderFrame frame();

private:
    void refresh_instances();

    Registry state;
    flecs::query<const RenderInstance> instanceQuery;
    Entity cameraEntity;
    std::vector<RenderInstance> instances;
    std::uint64_t instancesRevision = 0;
    Camera cachedCamera;
    Matrix4 cachedView;
    std::uint64_t cachedCameraRevision = 0;
};

}
