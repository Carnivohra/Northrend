#pragma once

#include "World/Map/selection.hpp"

#include <exception>
#include <memory>

namespace Northrend {

class MapLayout;

class MapResult final {
public:
    MapResult(MapSelection selection, std::shared_ptr<const MapLayout> layout) noexcept;
    MapResult(MapSelection selection, std::exception_ptr failure) noexcept;

    [[nodiscard]] const MapSelection& selection() const noexcept;
    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] std::shared_ptr<const MapLayout> data() && noexcept;
    void rethrow_if_failed() const;

private:
    MapSelection mapSelection;
    std::shared_ptr<const MapLayout> mapLayout;
    std::exception_ptr mapFailure;
};

}
