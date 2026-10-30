#pragma once

#include "World/Map/entry.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

class DbcStorage;

class MapCatalog final {
public:
    void tick(const DbcStorage& databases);

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const MapEntry* find(std::uint32_t id) const noexcept;

private:
    std::vector<MapEntry> maps;
    bool loaded = false;
};

}
