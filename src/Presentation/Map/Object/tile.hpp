#pragma once

#include "ECS/entity.hpp"
#include "Presentation/Map/Object/instance.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

class PresentationMapObjectTile final {
public:
    void reset(std::size_t objectCount);

    std::vector<PresentationMapObjectInstance> instances;
    std::vector<Entity> entities;
    std::vector<std::uint32_t> entityGroups;
    std::vector<bool> entityVisibility;
    std::vector<std::size_t> firstEntities;
    std::vector<std::size_t> entityCounts;
    std::vector<bool> rendered;
    std::vector<bool> completed;
    std::size_t remaining = 0;
};

}
