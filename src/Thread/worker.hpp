#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <utility>

namespace Northrend {

class Worker final {
public:
    template <typename Function>
    explicit Worker(Function&& function) :
        thread([this, function = std::forward<Function>(function)]() mutable { std::invoke(function, *this); }) {}

    ~Worker();

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    Worker(Worker&&) = delete;
    Worker& operator=(Worker&&) = delete;

    void request_stop() noexcept;
    void join() noexcept;

    [[nodiscard]] bool stopping() const noexcept;
    [[nodiscard]] bool joinable() const noexcept;

private:
    std::atomic_bool stopRequested = false;
    std::thread thread;
};

}
