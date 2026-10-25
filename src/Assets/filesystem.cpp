#include "Assets/filesystem.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace Northrend {

FilesystemAssetSource::FilesystemAssetSource(std::filesystem::path root) {
    std::error_code error;
    this->root = std::filesystem::weakly_canonical(root, error);

    if (error || !std::filesystem::is_directory(this->root, error) || error) {
        throw std::invalid_argument("Asset source root is not a readable directory");
    }
}

bool FilesystemAssetSource::contains(const AssetId& asset) const {
    std::error_code error;
    const bool exists = std::filesystem::is_regular_file(
        this->resolve(asset),
        error);

    return exists && !error;
}

AssetData FilesystemAssetSource::read(const AssetId& asset) const {
    const std::filesystem::path path = this->resolve(asset);
    std::ifstream stream(path, std::ios::binary | std::ios::ate);

    if (!stream) {
        throw std::runtime_error(
            "Failed to open asset: " + path.string());
    }

    const std::streampos end = stream.tellg();

    if (end < 0) {
        throw std::runtime_error(
            "Failed to determine asset size: " + path.string());
    }

    const auto length = static_cast<std::uintmax_t>(end);

    if (length > std::numeric_limits<std::size_t>::max() ||
        length > static_cast<std::uintmax_t>(
            std::numeric_limits<std::streamsize>::max())) {
        throw std::length_error(
            "Asset is too large to load: " + path.string());
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(length));
    stream.seekg(0, std::ios::beg);

    if (!bytes.empty()) {
        stream.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    }

    if (!stream) {
        throw std::runtime_error(
            "Failed to read asset: " + path.string());
    }

    return AssetData(std::move(bytes));
}

std::filesystem::path FilesystemAssetSource::resolve(
    const AssetId& asset) const {
    return this->root / std::filesystem::path(asset.path());
}

}
