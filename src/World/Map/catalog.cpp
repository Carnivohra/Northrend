#include "World/Map/catalog.hpp"

#include "Asset/Dbc/table.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace Northrend {

MapCatalog::MapCatalog(const DbcTable& table) {
    if (table.field_count() < 2) {
        throw std::runtime_error("Map.dbc does not contain the required fields");
    }

    std::vector<MapEntry> maps;
    maps.reserve(table.record_count());

    for (std::size_t record = 0; record < table.record_count(); ++record) {
        const std::uint32_t id = table.field(record, 0);
        const std::string_view directory = table.string(table.field(record, 1));

        if (directory.empty()) {
            throw std::runtime_error("Map.dbc contains an empty map directory");
        }

        maps.emplace_back(id, std::string(directory));
    }

    std::ranges::sort(maps, {}, &MapEntry::id);
    const auto duplicate = std::ranges::adjacent_find(maps, {}, &MapEntry::id);

    if (duplicate != maps.end()) {
        throw std::runtime_error("Map.dbc contains duplicate map identifiers");
    }

    this->maps = std::move(maps);
    this->loaded = true;
}

bool MapCatalog::ready() const noexcept {
    return this->loaded;
}

std::size_t MapCatalog::size() const noexcept {
    return this->maps.size();
}

const MapEntry* MapCatalog::find(const std::uint32_t id) const noexcept {
    if (!this->loaded) {
        return nullptr;
    }

    const auto position = std::ranges::lower_bound(this->maps, id, {}, &MapEntry::id);
    return position != this->maps.end() && position->id() == id ? &*position : nullptr;
}

}
