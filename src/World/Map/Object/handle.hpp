#pragma once

#include <memory>
#include <string_view>

namespace Northrend {

class MapObjectAsset;
class MapObjectState;

class MapObjectHandle final {
public:
    MapObjectHandle() noexcept = default;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::string_view asset() const noexcept;
    [[nodiscard]] std::shared_ptr<const MapObjectAsset> data() const;

private:
    friend class MapObjectLoader;

    explicit MapObjectHandle(std::shared_ptr<MapObjectState> state) noexcept;

    std::shared_ptr<MapObjectState> state;
};

}
