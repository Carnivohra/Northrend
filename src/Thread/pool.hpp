#pragma once

#include "Thread/work.hpp"

#include <array>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace Northrend {

class Worker;

class ThreadPool final {
public:
    explicit ThreadPool(std::size_t workerCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    [[nodiscard]] std::shared_ptr<Work> submit(WorkPriority priority, Work::Function function);
    [[nodiscard]] bool promote(const std::shared_ptr<Work>& work, WorkPriority priority);
    [[nodiscard]] std::size_t worker_count() const noexcept;

    void stop() noexcept;

private:
    static constexpr std::size_t PriorityCount = 3;

    [[nodiscard]] static std::size_t priority_index(WorkPriority priority);
    [[nodiscard]] bool has_work() const noexcept;
    [[nodiscard]] std::shared_ptr<Work> next_work();
    void run(Worker& worker);

    mutable std::mutex mutex;
    std::condition_variable wake;
    std::array<std::deque<std::shared_ptr<Work>>, PriorityCount> queues;
    std::vector<std::unique_ptr<Worker>> workers;
    const std::size_t workerCount;
    bool stopping = false;
};

}
