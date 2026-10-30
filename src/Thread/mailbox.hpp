#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

namespace Northrend {

template <typename Type, std::size_t Capacity>
    requires std::move_constructible<Type>
class Mailbox final {
public:
    static_assert(Capacity > 0 && std::has_single_bit(Capacity), "Mailbox capacity must be a power of two");
    static_assert(std::atomic<std::size_t>::is_always_lock_free, "Mailbox indices must be lock-free");

    Mailbox() = default;

    Mailbox(const Mailbox&) = delete;
    Mailbox& operator=(const Mailbox&) = delete;
    Mailbox(Mailbox&&) = delete;
    Mailbox& operator=(Mailbox&&) = delete;

    template <typename... Arguments>
        requires std::constructible_from<Type, Arguments...>
    [[nodiscard]] bool try_emplace(Arguments&&... arguments) {
        const std::size_t write = this->writePosition.load(std::memory_order_relaxed);
        const std::size_t read = this->readPosition.load(std::memory_order_acquire);

        if (write - read >= Capacity) {
            return false;
        }

        this->slots[write & Mask].emplace(std::forward<Arguments>(arguments)...);
        this->writePosition.store(write + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool try_push(const Type& value)
        requires std::copy_constructible<Type>
    {
        return this->try_emplace(value);
    }

    [[nodiscard]] bool try_push(Type&& value) {
        return this->try_emplace(std::move(value));
    }

    [[nodiscard]] std::optional<Type> try_pop() {
        const std::size_t read = this->readPosition.load(std::memory_order_relaxed);
        const std::size_t write = this->writePosition.load(std::memory_order_acquire);

        if (read == write) {
            return std::nullopt;
        }

        std::optional<Type>& slot = this->slots[read & Mask];
        std::optional<Type> value(std::move(*slot));
        slot.reset();
        this->readPosition.store(read + 1, std::memory_order_release);
        return value;
    }

    void clear() {
        while (this->try_pop()) {
        }
    }

    [[nodiscard]] bool empty() const noexcept {
        return this->size() == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        return this->size() == Capacity;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        const std::size_t read = this->readPosition.load(std::memory_order_acquire);
        const std::size_t write = this->writePosition.load(std::memory_order_acquire);
        return write - read;
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

private:
    static constexpr std::size_t Mask = Capacity - 1;
    static constexpr std::size_t CacheLineSize = 64;

    std::array<std::optional<Type>, Capacity> slots;
    alignas(CacheLineSize) std::atomic<std::size_t> readPosition {0};
    alignas(CacheLineSize) std::atomic<std::size_t> writePosition {0};
};

}
