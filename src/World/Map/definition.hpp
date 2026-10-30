#pragma once

#include "World/Map/selection.hpp"

#include <exception>
#include <memory>

namespace Northrend {

class Wdt;

class MapDefinition final {
public:
    MapDefinition(MapSelection selection, std::shared_ptr<const Wdt> wdt) noexcept;
    MapDefinition(MapSelection selection, std::exception_ptr failure) noexcept;

    [[nodiscard]] const MapSelection& selection() const noexcept;
    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] std::shared_ptr<const Wdt> data() && noexcept;
    void rethrow_if_failed() const;

private:
    MapSelection mapSelection;
    std::shared_ptr<const Wdt> mapData;
    std::exception_ptr mapFailure;
};

}
