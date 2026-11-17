#pragma once

#include "Asset/Format/Wmo/group.hpp"
#include "Asset/Format/Wmo/wmo.hpp"

#include <memory>
#include <span>
#include <vector>

namespace Northrend {

class MapObjectAsset final {
public:
    MapObjectAsset(std::unique_ptr<Wmo> root, std::vector<WmoGroup> groups);

    [[nodiscard]] const Wmo& root() const noexcept;
    [[nodiscard]] std::span<const WmoGroup> groups() const noexcept;

private:
    std::unique_ptr<Wmo> rootData;
    std::vector<WmoGroup> groupData;
};

}
