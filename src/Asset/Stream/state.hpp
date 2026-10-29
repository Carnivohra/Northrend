#pragma once

#include "Asset/Stream/priority.hpp"
#include "Asset/data.hpp"
#include "Asset/id.hpp"

#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>

namespace Northrend {

class AssetState final {
public:
    AssetState(AssetId asset, AssetPriority priority);

    [[nodiscard]] const AssetId& asset() const noexcept;
    [[nodiscard]] AssetPriority priority() const noexcept;
    void set_priority(AssetPriority priority) noexcept;

    [[nodiscard]] bool queued() const;
    [[nodiscard]] bool begin();
    void complete(std::shared_ptr<const AssetData> data);
    void fail(std::exception_ptr failure);
    void cancel();

    [[nodiscard]] bool ready() const;
    [[nodiscard]] std::shared_ptr<const AssetData> data() const;

private:
    enum class Status : std::uint8_t { queued, loading, ready, failed, cancelled };

    AssetId identifier;
    AssetPriority requestPriority;
    mutable std::mutex mutex;
    mutable std::condition_variable wake;
    Status status = Status::queued;
    std::shared_ptr<const AssetData> result;
    std::exception_ptr failure;
};

}
