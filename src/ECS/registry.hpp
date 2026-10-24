#pragma once

#include "ECS/entity.hpp"

#include <flecs.h>

#include <chrono>
#include <concepts>
#include <utility>

namespace Northrend {

class Registry final {
public:
    Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;

    [[nodiscard]] Entity create();
    void destroy(Entity entity);

    [[nodiscard]] bool alive(Entity entity) const noexcept;
    void progress(std::chrono::duration<float> delta);

    template <typename... Components, typename Callback>
    void add_system(Callback&& callback) {
        this->world.system<Components...>().each(
            std::forward<Callback>(callback));
    }

    template <typename... Components>
    [[nodiscard]] flecs::query<Components...> tracked_query() {
        return this->world.query_builder<Components...>()
            .cached()
            .detect_changes()
            .build();
    }

    template <typename Component, typename... Arguments>
        requires std::constructible_from<Component, Arguments...>
    Component& emplace(Entity entity, Arguments&&... arguments) {
        auto handle = this->world.entity(entity.id());
        handle.emplace<Component>(std::forward<Arguments>(arguments)...);
        return handle.get_mut<Component>();
    }

    template <typename Component>
    void remove(Entity entity) {
        this->world.entity(entity.id()).remove<Component>();
    }

    template <typename Component>
    [[nodiscard]] bool contains(Entity entity) const {
        return this->world.entity(entity.id()).has<Component>();
    }

    template <typename Component>
    [[nodiscard]] const Component* get(Entity entity) const {
        return this->world.entity(entity.id()).try_get<Component>();
    }

    template <typename Component>
    [[nodiscard]] Component* get(Entity entity) {
        auto handle = this->world.entity(entity.id());
        Component* component = handle.try_get_mut<Component>();

        if (component) {
            handle.modified<Component>();
        }

        return component;
    }

private:
    flecs::world world;
};

}
