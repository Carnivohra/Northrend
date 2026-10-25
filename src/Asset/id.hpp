#pragma once

#include <string>
#include <string_view>

namespace Northrend {

class AssetId final {
public:
    explicit AssetId(std::string_view path);

    [[nodiscard]] std::string_view path() const noexcept;

    bool operator==(const AssetId&) const = default;

private:
    std::string value;
};

}
