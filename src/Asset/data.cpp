#include "Asset/data.hpp"

#include <utility>

namespace Northrend {

AssetData::AssetData(std::vector<std::byte> bytes) noexcept
    : storage(std::move(bytes)) {}

std::span<const std::byte> AssetData::bytes() const noexcept {
    return this->storage;
}

std::size_t AssetData::size() const noexcept {
    return this->storage.size();
}

bool AssetData::empty() const noexcept {
    return this->storage.empty();
}

}
