#pragma once

#include "Thread/mailbox.hpp"
#include "World/Map/Tile/change.hpp"
#include "World/snapshot.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace Northrend {

class PresentationExchange final {
public:
    static constexpr std::size_t TileCapacity = 4096;

    PresentationExchange() = default;

    PresentationExchange(const PresentationExchange&) = delete;
    PresentationExchange& operator=(const PresentationExchange&) = delete;

    [[nodiscard]] WorldSnapshot& write() noexcept {
        return this->snapshots[this->producerIndex];
    }

    void publish() noexcept {
        const std::uint8_t previous =
            this->available.exchange(encode(this->producerIndex, true), std::memory_order_acq_rel);
        this->producerIndex = decode_index(previous);
    }

    [[nodiscard]] const WorldSnapshot& read_latest() noexcept {
        const std::uint8_t available = this->available.load(std::memory_order_acquire);

        if (is_pending(available)) {
            const std::uint8_t previous =
                this->available.exchange(encode(this->consumerIndex, false), std::memory_order_acq_rel);
            this->consumerIndex = decode_index(previous);
        }

        return this->snapshots[this->consumerIndex];
    }

    [[nodiscard]] bool publish(TileChange change) {
        return this->tileChanges.try_push(std::move(change));
    }

    [[nodiscard]] std::optional<TileChange> next_tile() {
        return this->tileChanges.try_pop();
    }

    [[nodiscard]] bool tiles_full() const noexcept {
        return this->tileChanges.full();
    }

private:
    static constexpr std::uint8_t PendingBit = 0x80;
    static constexpr std::uint8_t IndexMask = 0x03;

    static constexpr std::uint8_t encode(std::size_t index, bool pending) noexcept {
        return static_cast<std::uint8_t>(index) | (pending ? PendingBit : 0);
    }

    static constexpr std::size_t decode_index(std::uint8_t value) noexcept {
        return value & IndexMask;
    }

    static constexpr bool is_pending(std::uint8_t value) noexcept {
        return (value & PendingBit) != 0;
    }

    std::array<WorldSnapshot, 3> snapshots {};
    Mailbox<TileChange, TileCapacity> tileChanges;
    std::atomic<std::uint8_t> available = encode(1, false);
    std::size_t consumerIndex = 0;
    std::size_t producerIndex = 2;
};

}
