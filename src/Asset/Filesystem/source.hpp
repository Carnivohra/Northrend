#pragma once

#include "Asset/source.hpp"

#include <filesystem>

namespace Northrend {

class FilesystemAssetSource final : public AssetSource {
public:
    explicit FilesystemAssetSource(std::filesystem::path root);

    [[nodiscard]] bool contains(const AssetId& asset) const override;
    [[nodiscard]] AssetData read(const AssetId& asset) const override;

private:
    [[nodiscard]] std::filesystem::path resolve(const AssetId& asset) const;

    std::filesystem::path root;
};

}
