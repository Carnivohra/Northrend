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
    std::uint32_t sidnColor = 0;
    std::uint32_t frameSidnColor = 0;
    std::uint32_t diffuseColor = 0;
    std::uint32_t groundType = 0;
    std::uint32_t color2 = 0;
    std::uint32_t flags2 = 0;
};

}
