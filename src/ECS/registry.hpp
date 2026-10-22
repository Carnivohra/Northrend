#pragma once

#include "ECS/entity.hpp"

#include <flecs.h>

#include <concepts>
#include <utility>

namespace Northrend {

class Registry final {
public:
    Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    [[nodiscard]] Entity create();
    void destroy(Entity entity);

    [[nodiscard]] bool alive(Entity entity) const noexcept;

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
        return this->world.entity(entity.id()).try_get_mut<Component>();
    }

private:
    flecs::world world;
};

}
