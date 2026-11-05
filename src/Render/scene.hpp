#pragma once

#include "ECS/registry.hpp"
#include "Render/camera.hpp"
#include "Render/frame.hpp"
#include "Render/instance.hpp"
#include "Render/light.hpp"

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
    [[nodiscard]] const DirectionalLight& active_light() const;

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

    template <typename Callback>
        requires std::invocable<Callback, DirectionalLight&>
    void modify_active_light(Callback&& callback) {
        const bool modified =
            this->state.modify<DirectionalLight>(this->lightEntity, [&callback](DirectionalLight& light) {
                std::invoke(std::forward<Callback>(callback), light);
                ++light.revision;
            });

        if (!modified) {
            throw std::runtime_error("Render scene has no active directional light");
        }
    }

    [[nodiscard]] RenderFrame frame();

private:
    void refresh_instances();

    Registry state;
    flecs::query<const RenderInstance> instanceQuery;
    Entity cameraEntity;
    Entity lightEntity;
    std::vector<RenderInstance> instances;
    std::uint64_t instancesRevision = 0;
    Camera cachedCamera;
    Matrix4 cachedView;
    std::uint64_t cachedCameraRevision = 0;
};

}
