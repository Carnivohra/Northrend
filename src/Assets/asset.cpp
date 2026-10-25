#include "Assets/asset.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace Northrend {

AssetId::AssetId(std::string_view path)
    : value(path) {
    std::ranges::replace(this->value, '\\', '/');

    std::filesystem::path normalized(this->value);

    if (normalized.empty() ||
        normalized.is_absolute() ||
        normalized.has_root_path()) {
        throw std::invalid_argument("Asset path must be relative");
    }

    normalized = normalized.lexically_normal();

    for (const std::filesystem::path& component : normalized) {
        if (component == "..") {
            throw std::invalid_argument("Asset path must remain inside its source");
        }
    }

    this->value = normalized.generic_string();

    if (this->value.empty() || this->value == ".") {
        throw std::invalid_argument("Asset path must identify a file");
    }
}

std::string_view AssetId::path() const noexcept {
    return this->value;
}

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
