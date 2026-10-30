#pragma once

#include "Thread/mailbox.hpp"
#include "World/Map/catalog.hpp"
#include "World/Map/result.hpp"
#include "World/Map/selection.hpp"
#include "World/Map/Tile/request.hpp"
#include "World/Map/Tile/result.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Northrend {

class MapExchange final {
public:
    static constexpr std::size_t SelectionCapacity = 8;
    static constexpr std::size_t TileCapacity = 4096;

    [[nodiscard]] bool submit(std::string_view map, const MapSelection& selection);
    [[nodiscard]] bool submit(const TileRequest& request);

    [[nodiscard]] std::optional<MapCatalog> catalog();
    [[nodiscard]] std::optional<MapResult> map();
    [[nodiscard]] std::optional<TileResult> tile();

    [[nodiscard]] std::optional<std::pair<MapSelection, std::string>> next_selection();
    [[nodiscard]] std::optional<TileRequest> next_tile_request();

    [[nodiscard]] bool publish(MapCatalog catalog);
    [[nodiscard]] bool publish(MapResult result);
    [[nodiscard]] bool publish(TileResult result);
    [[nodiscard]] bool map_full() const noexcept;
    [[nodiscard]] bool tiles_full() const noexcept;

    void clear();

private:
    Mailbox<std::pair<MapSelection, std::string>, SelectionCapacity> selections;
    Mailbox<TileRequest, TileCapacity> tileRequests;
    Mailbox<MapCatalog, 1> catalogs;
    Mailbox<MapResult, SelectionCapacity> maps;
    Mailbox<TileResult, TileCapacity> tiles;
};

}
