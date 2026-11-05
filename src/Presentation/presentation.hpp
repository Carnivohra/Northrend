#pragma once

#include "Backend/Window/window.hpp"
#include "Input/input.hpp"
#include "Presentation/exchange.hpp"
#include "Presentation/Map/map.hpp"
#include "Render/Mesh/allocator.hpp"
#include "Render/renderer.hpp"
#include "Render/scene.hpp"
#include "World/snapshot.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

namespace Northrend {

template <typename Type>
concept PresentationRenderer = std::derived_from<Type, Renderer> && std::constructible_from<Type, const Window&>;

template <PresentationRenderer RendererType> class Presentation final {
public:
    explicit Presentation(PresentationExchange& presentationExchange) : presentationExchange(presentationExchange) {}

    void start(const Window& window) {
        this->renderer.emplace(window);
        this->window = &window;
    }

    void suspend() {
        if (this->renderer) {
            this->renderer->suspend();
        }
    }

    void tick(const Input& input) {
        bool mapChanged = false;

        for (std::size_t count = 0; count < TileChangesPerFrame; ++count) {
            std::optional<TileChange> change = this->presentationExchange.next_tile();

            if (!change) {
                break;
            }

            this->map.apply(std::move(*change), this->meshHandles, *this->renderer, this->scene);
            mapChanged = true;
        }

        const WorldSnapshot& snapshot = this->presentationExchange.read_latest();

        if (snapshot.sequence != this->latestSnapshot.sequence) {
            this->latestSnapshot = snapshot;
        }

        this->control_camera(input, mapChanged);
        this->map.update(this->scene.active_camera(), this->window->width(), this->window->height(), this->scene);
        this->renderer->frame(this->scene.frame());
    }

    void stop() {
        if (this->renderer) {
            this->map.clear(*this->renderer, this->scene);
        }

        this->renderer.reset();
        this->window = nullptr;
    }

private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t TileChangesPerFrame = 32;
    static constexpr float CameraPitchLimit = 1.55334306F;

    [[nodiscard]] static Vector3 direction(float yaw, float pitch) noexcept {
        const float horizontal = std::cos(pitch);
        return Vector3 {.x = horizontal * std::cos(yaw), .y = horizontal * std::sin(yaw), .z = std::sin(pitch)};
    }

    void control_camera(const Input& input, bool mapChanged) {
        const Clock::time_point now = Clock::now();
        const float delta = std::clamp(std::chrono::duration<float>(now - this->cameraTick).count(), 0.0F, 0.05F);
        this->cameraTick = now;

        const Keyboard& keyboard = input.keyboard();
        const Mouse& mouse = input.mouse();
        const bool movement = keyboard.down(Key::w) || keyboard.down(Key::s) || keyboard.down(Key::a) ||
            keyboard.down(Key::d) || keyboard.down(Key::space) || keyboard.down(Key::leftControl);
        const bool rotation = mouse.down(MouseButton::right) && (mouse.delta_x() != 0.0F || mouse.delta_y() != 0.0F);
        const bool speedChanged = mouse.wheel_y() != 0.0F;

        if (!this->cameraControlled && !movement && !rotation && !speedChanged) {
            return;
        }

        if (!this->cameraControlled) {
            const Camera& camera = this->scene.active_camera();
            const Vector3 view = camera.forward();
            this->cameraPosition = camera.position;
            this->cameraYaw = std::atan2(view.y, view.x);
            this->cameraPitch = std::asin(std::clamp(view.z, -1.0F, 1.0F));
            this->cameraControlled = true;
        }

        if (rotation) {
            this->cameraYaw -= mouse.delta_x() * 0.0025F;
            this->cameraPitch =
                std::clamp(this->cameraPitch - mouse.delta_y() * 0.0025F, -CameraPitchLimit, CameraPitchLimit);
        }

        if (speedChanged) {
            this->cameraSpeed = std::clamp(this->cameraSpeed * std::exp(mouse.wheel_y() * 0.2F), 10.0F, 100'000.0F);
        }

        const Vector3 view = direction(this->cameraYaw, this->cameraPitch);

        if (movement) {
            const float rightLength = std::sqrt(view.x * view.x + view.y * view.y);
            const Vector3 right {
                .x = rightLength > 0.0F ? view.y / rightLength : 0.0F,
                .y = rightLength > 0.0F ? -view.x / rightLength : 1.0F
            };
            Vector3 movementDirection;

            if (keyboard.down(Key::w)) {
                movementDirection.x += view.x;
                movementDirection.y += view.y;
                movementDirection.z += view.z;
            }

            if (keyboard.down(Key::s)) {
                movementDirection.x -= view.x;
                movementDirection.y -= view.y;
                movementDirection.z -= view.z;
            }

            if (keyboard.down(Key::d)) {
                movementDirection.x += right.x;
                movementDirection.y += right.y;
            }

            if (keyboard.down(Key::a)) {
                movementDirection.x -= right.x;
                movementDirection.y -= right.y;
            }

            if (keyboard.down(Key::space)) {
                movementDirection.z += 1.0F;
            }

            if (keyboard.down(Key::leftControl)) {
                movementDirection.z -= 1.0F;
            }

            const float movementLength = std::sqrt(movementDirection.x * movementDirection.x +
                movementDirection.y * movementDirection.y + movementDirection.z * movementDirection.z);

            if (movementLength > 0.0F) {
                const float boost = keyboard.down(Key::leftShift) ? 4.0F : 1.0F;
                const float distance = this->cameraSpeed * boost * delta / movementLength;
                this->cameraPosition.x += movementDirection.x * distance;
                this->cameraPosition.y += movementDirection.y * distance;
                this->cameraPosition.z += movementDirection.z * distance;
            }
        }

        if (!movement && !rotation && !speedChanged && !mapChanged) {
            return;
        }

        this->scene.modify_active_camera([this, view](Camera& camera) {
            camera.position = this->cameraPosition;
            camera.nearPlane = 0.5F;
            camera.farPlane = 250'000.0F;
            camera.look_at(Vector3 {
                .x = this->cameraPosition.x + view.x,
                .y = this->cameraPosition.y + view.y,
                .z = this->cameraPosition.z + view.z
            });
        });
    }

    PresentationExchange& presentationExchange;
    WorldSnapshot latestSnapshot;
    MeshHandleAllocator meshHandles;
    PresentationMap map;
    RenderScene scene;
    std::optional<RendererType> renderer;
    Clock::time_point cameraTick = Clock::now();
    Vector3 cameraPosition;
    float cameraYaw = 0.0F;
    float cameraPitch = 0.0F;
    float cameraSpeed = 2'000.0F;
    bool cameraControlled = false;
    const Window* window = nullptr;
};

}
