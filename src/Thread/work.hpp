#pragma once

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>

namespace Northrend {

enum class WorkPriority : std::uint8_t { background, normal, critical };

class ThreadPool;

class Work final {
public:
    using Function = std::function<void()>;

    Work(const Work&) = delete;
    Work& operator=(const Work&) = delete;
    Work(Work&&) = delete;
    Work& operator=(Work&&) = delete;

    void cancel() noexcept;

    [[nodiscard]] bool ready() const noexcept;
    void wait() const;

private:
    friend class ThreadPool;

    enum class Status : std::uint8_t { queued, running, cancelling, complete, failed, cancelled };

    Work(WorkPriority priority, Function function);

    [[nodiscard]] bool queued() const noexcept;
    void execute() noexcept;

    WorkPriority priority;
    Function function;
    mutable std::mutex mutex;
    mutable std::condition_variable wake;
    Status status = Status::queued;
    std::exception_ptr failure;
};

}
