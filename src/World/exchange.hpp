#pragma once

#include "Thread/mailbox.hpp"
#include "World/Map/catalog.hpp"
#include "World/Map/definition.hpp"
#include "World/Map/request.hpp"
#include "World/Map/result.hpp"
#include "World/Map/selection.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Northrend {

class WorldExchange final {
public:
    static constexpr std::size_t SelectionCapacity = 8;
    static constexpr std::size_t TerrainCapacity = 4096;

    [[nodiscard]] bool submit(std::string_view map, const MapSelection& selection);
    [[nodiscard]] bool submit(const TerrainRequest& request);

    [[nodiscard]] std::optional<MapCatalog> catalog();
    [[nodiscard]] std::optional<MapDefinition> definition();
    [[nodiscard]] std::optional<TerrainResult> result();

    [[nodiscard]] std::optional<std::pair<MapSelection, std::string>> next_selection();
    [[nodiscard]] std::optional<TerrainRequest> next_request();

    [[nodiscard]] bool publish(MapCatalog catalog);
    [[nodiscard]] bool publish(MapDefinition definition);
    [[nodiscard]] bool publish(TerrainResult result);
    [[nodiscard]] bool definition_full() const noexcept;
    [[nodiscard]] bool result_full() const noexcept;

    void clear();

private:
    Mailbox<std::pair<MapSelection, std::string>, SelectionCapacity> selections;
    Mailbox<TerrainRequest, TerrainCapacity> requests;
    Mailbox<MapCatalog, 1> catalogs;
    Mailbox<MapDefinition, SelectionCapacity> definitions;
    Mailbox<TerrainResult, TerrainCapacity> results;
};

}
