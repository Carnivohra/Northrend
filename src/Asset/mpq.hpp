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

    [[nodiscard]] std::vector<AssetId> entries(std::stop_token stopToken = {}) const override;
    [[nodiscard]] bool contains(const AssetId& asset) const override;
    [[nodiscard]] std::optional<AssetData> try_read(const AssetId& asset) const override;

private:
    [[nodiscard]] std::optional<AssetData> read_entry(const std::string& name) const;
    [[nodiscard]] static std::string entry_name(const AssetId& asset);

    std::filesystem::path path;
    void* handle = nullptr;
    mutable std::mutex access;
};

}
