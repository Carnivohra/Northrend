#pragma once

#include "Asset/Stream/priority.hpp"
#include "Asset/id.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>

namespace Northrend {

class MapObjectAsset;

class MapObjectState final {
public:
    MapObjectState(AssetId asset, AssetPriority priority, std::uint32_t generation);

    [[nodiscard]] const AssetId& asset() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;
    [[nodiscard]] AssetPriority priority() const noexcept;
    void promote(AssetPriority priority) noexcept;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::shared_ptr<const MapObjectAsset> data() const;
    void complete(std::shared_ptr<const MapObjectAsset> data);
    void fail(std::exception_ptr failure) noexcept;
    void cancel() noexcept;

private:
    enum class Status : std::uint8_t { loading, ready, failed, cancelled };

    AssetId identifier;
    mutable std::mutex mutex;
    std::shared_ptr<const MapObjectAsset> result;
    std::exception_ptr failure;
    AssetPriority requestPriority;
    std::uint32_t mapGeneration;
    Status status = Status::loading;
};

}
