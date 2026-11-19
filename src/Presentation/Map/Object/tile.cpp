#include "Presentation/Map/Object/tile.hpp"

namespace Northrend {

void PresentationMapObjectTile::reset(const std::size_t objectCount) {
    this->instances.clear();
    this->entities.clear();
    this->entityGroups.clear();
    this->entityVisibility.clear();
    this->firstEntities.clear();
    this->entityCounts.clear();
    this->rendered.clear();
    this->completed.assign(objectCount, false);
    this->remaining = objectCount;

    this->instances.reserve(objectCount);
    this->firstEntities.reserve(objectCount);
    this->entityCounts.reserve(objectCount);
    this->rendered.reserve(objectCount);
}

}
