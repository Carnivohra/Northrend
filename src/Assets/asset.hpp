#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Northrend {

class AssetId final {
public:
    explicit AssetId(std::string_view path);

    [[nodiscard]] std::string_view path() const noexcept;

    bool operator==(const AssetId&) const = default;

private:
    std::string value;
};

class AssetData final {
public:
    AssetData() = default;
    explicit AssetData(std::vector<std::byte> bytes) noexcept;

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::vector<std::byte> storage;
};

}
