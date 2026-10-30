#include "World/Map/entry.hpp"

#include <utility>

namespace Northrend {

MapEntry::MapEntry(const std::uint32_t id, std::string directory) :
    identifier(id), directoryName(std::move(directory)) {}

std::uint32_t MapEntry::id() const noexcept {
    return this->identifier;
}

std::string_view MapEntry::directory() const noexcept {
    return this->directoryName;
}

}
