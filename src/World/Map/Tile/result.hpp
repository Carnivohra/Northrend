#pragma once

#include "World/Map/Tile/request.hpp"

#include <exception>
#include <memory>

namespace Northrend {

class MapTile;

class TileResult final {
public:
    TileResult(TileRequest request, std::shared_ptr<const MapTile> tile) noexcept;
    TileResult(TileRequest request, std::exception_ptr failure) noexcept;

    [[nodiscard]] const TileRequest& request() const noexcept;
    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] const std::shared_ptr<const MapTile>& data() const noexcept;
    [[nodiscard]] std::shared_ptr<const MapTile> data() && noexcept;
    void rethrow_if_failed() const;

private:
    TileRequest tileRequest;
    std::shared_ptr<const MapTile> mapTile;
    std::exception_ptr tileFailure;
};

}
