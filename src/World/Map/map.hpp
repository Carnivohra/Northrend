#pragma once

#include "World/Map/catalog.hpp"
#include "World/Map/definition.hpp"
#include "World/Map/selection.hpp"
#include "World/Map/terrain.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Northrend {

enum class MapState : std::uint8_t { empty, requested, loading, ready, failed };

class Map final {
public:
    [[nodiscard]] MapCatalog& catalog() noexcept;
    [[nodiscard]] const MapCatalog& catalog() const noexcept;

    void select(std::uint32_t map);
    [[nodiscard]] std::optional<MapSelection> next_selection() noexcept;
    [[nodiscard]] bool defer(const MapSelection& selection) noexcept;
    [[nodiscard]] bool complete(MapDefinition definition) noexcept;

    [[nodiscard]] Terrain* terrain() noexcept;
    [[nodiscard]] const Terrain* terrain() const noexcept;
    [[nodiscard]] std::string_view directory() const noexcept;
    [[nodiscard]] MapState state() const noexcept;
    [[nodiscard]] std::uint32_t id() const noexcept;
    void clear() noexcept;

private:
    [[nodiscard]] static std::uint32_t next_generation(std::uint32_t generation) noexcept;

    MapCatalog mapCatalog;
    std::optional<Terrain> mapTerrain;
    std::string mapDirectory;
    std::uint32_t nextTerrainGeneration = 1;
    std::uint32_t activeGeneration = 0;
    std::uint32_t activeIdentifier = 0;
    MapState mapState = MapState::empty;
};

}
