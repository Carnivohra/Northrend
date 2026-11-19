#pragma once

#include "Asset/id.hpp"

#include <cstdint>
#include <optional>

namespace Northrend {

struct PresentationMapObjectMaterial final {
    std::optional<AssetId> texture1;
    std::optional<AssetId> texture2;
    std::uint32_t flags = 0;
    std::uint32_t shader = 0;
    std::uint32_t blendMode = 0;
};

}
