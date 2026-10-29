#include "Asset/directory.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace Northrend {

AssetDirectory::AssetDirectory(std::filesystem::path root) {
    std::error_code error;
    this->root = std::filesystem::weakly_canonical(root, error);

    if (error || !std::filesystem::is_directory(this->root, error) || error) {
        throw std::invalid_argument("Asset source root is not a readable directory");
    }
}

std::vector<AssetId> AssetDirectory::entries(const std::stop_token stopToken) const {
    std::vector<AssetId> entries;
    std::error_code error;
    std::filesystem::recursive_directory_iterator position(
        this->root, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end;

    while (!stopToken.stop_requested() && !error && position != end) {
        const std::filesystem::directory_entry& entry = *position;

        if (!entry.is_symlink(error) && entry.is_regular_file(error)) {
            const std::filesystem::path relative = std::filesystem::relative(entry.path(), this->root, error);

            if (!error) {
                entries.emplace_back(relative.generic_string());
            }
        }

        position.increment(error);
    }

    if (error) {
        throw std::runtime_error("Failed to enumerate asset directory: " + this->root.string());
    }

    return entries;
}

bool AssetDirectory::contains(const AssetId& asset) const {
    std::error_code error;
    const bool exists = std::filesystem::is_regular_file(this->resolve(asset), error);
    return exists && !error;
}

std::optional<AssetData> AssetDirectory::try_read(const AssetId& asset) const {
    const std::filesystem::path path = this->resolve(asset);
    std::ifstream stream(path, std::ios::binary | std::ios::ate);

    if (!stream) {
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);

        if (!exists && !error) {
            return std::nullopt;
        }

        throw std::runtime_error("Failed to open asset: " + path.string());
    }

    const std::streampos end = stream.tellg();

    if (end < 0) {
        throw std::runtime_error("Failed to determine asset size: " + path.string());
    }

    const auto length = static_cast<std::uintmax_t>(end);

    if (length > std::numeric_limits<std::size_t>::max() ||
        length > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::length_error("Asset is too large to load: " + path.string());
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(length));
    stream.seekg(0, std::ios::beg);

    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    if (!stream) {
        throw std::runtime_error("Failed to read asset: " + path.string());
    }

    return AssetData(std::move(bytes));
}

std::filesystem::path AssetDirectory::resolve(const AssetId& asset) const {
    return this->root / std::filesystem::path(asset.path());
}

}
