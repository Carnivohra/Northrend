#pragma once

#include "Asset/source.hpp"

#include <filesystem>

namespace Northrend {

class AssetDirectory final : public AssetSource {
public:
    explicit AssetDirectory(std::filesystem::path root);

    [[nodiscard]] bool contains(const AssetId& asset) const override;
    [[nodiscard]] AssetData read(const AssetId& asset) const override;

private:
    [[nodiscard]] std::filesystem::path resolve(const AssetId& asset) const;

    std::filesystem::path root;
};

}
