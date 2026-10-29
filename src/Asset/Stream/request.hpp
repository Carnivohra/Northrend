#pragma once

#include "Asset/data.hpp"

#include <memory>

namespace Northrend {

class AssetState;
class AssetStreamer;

class AssetRequest final {
public:
    AssetRequest() noexcept = default;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool ready() const;
    [[nodiscard]] std::shared_ptr<const AssetData> data() const;

private:
    friend class AssetStreamer;

    explicit AssetRequest(std::shared_ptr<AssetState> state) noexcept;

    std::shared_ptr<AssetState> state;
};

}
