#pragma once

#include "Asset/source.hpp"

#include <filesystem>
#include <mutex>
#include <string>

namespace Northrend {

class MpqArchive final : public AssetSource {
public:
    explicit MpqArchive(const std::filesystem::path& path);
    ~MpqArchive() override;

    MpqArchive(const MpqArchive&) = delete;
    MpqArchive& operator=(const MpqArchive&) = delete;
    MpqArchive(MpqArchive&&) = delete;
    MpqArchive& operator=(MpqArchive&&) = delete;

    [[nodiscard]] bool contains(const AssetId& asset) const override;
    [[nodiscard]] std::optional<AssetData> try_read(const AssetId& asset) const override;

private:
    [[nodiscard]] static std::string entry_name(const AssetId& asset);

    std::filesystem::path path;
    void* handle = nullptr;
    mutable std::mutex access;
};

}
