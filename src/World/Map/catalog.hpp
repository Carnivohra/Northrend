#pragma once

#include "World/Map/entry.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Northrend {

class DbcTable;

class MapCatalog final {
public:
    MapCatalog() = default;
    explicit MapCatalog(const DbcTable& table);

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const MapEntry* find(std::uint32_t id) const noexcept;

private:
    std::vector<MapEntry> maps;
    bool loaded = false;
};

}
