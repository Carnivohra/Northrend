#pragma once

#include "Thread/rate.hpp"
#include "Thread/task.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>

namespace Northrend {

template <ThreadTask Task>
class TaskRunner final {
public:
    explicit TaskRunner(Task task, TickRate tickRate)
        : tickRate(tickRate),
          tickInterval(tickRate.interval()),
          worker(
              [this, task = std::move(task)]() mutable {
                  this->run(task);
              }) {}

    ~TaskRunner() {
        this->stop_without_rethrow();
    }

    TaskRunner(const TaskRunner&) = delete;
    TaskRunner& operator=(const TaskRunner&) = delete;
    TaskRunner(TaskRunner&&) = delete;
    TaskRunner& operator=(TaskRunner&&) = delete;

    [[nodiscard]] bool running() const noexcept {
        const std::scoped_lock lock(this->mutex);
        return this->isRunning;
    }

    [[nodiscard]] bool paused() const noexcept {
        const std::scoped_lock lock(this->mutex);
        return this->isPaused;
    }

    [[nodiscard]] TickRate tick_rate() const noexcept {
        const std::scoped_lock lock(this->mutex);
        return this->tickRate;
    }

    void set_tick_rate(TickRate tickRate) noexcept {
        {
            const std::scoped_lock lock(this->mutex);

            if (this->tickRate == tickRate || !this->isRunning) {
                return;
            }

            this->tickRate = tickRate;
            this->tickInterval = tickRate.interval();
            ++this->revision;
        }

        this->wake.notify_all();
    }

    void pause() noexcept {
        {
            const std::scoped_lock lock(this->mutex);

            if (this->isPaused || !this->isRunning) {
                return;
            }

            this->isPaused = true;
            ++this->revision;
        }

        this->wake.notify_all();
    }

    void resume() noexcept {
        {
            const std::scoped_lock lock(this->mutex);

            if (!this->isPaused || !this->isRunning) {
                return;
            }

            this->isPaused = false;
            ++this->revision;
        }

        this->wake.notify_all();
    }

    void rethrow_if_failed() const {
        std::exception_ptr currentFailure;

        {
            const std::scoped_lock lock(this->mutex);
            currentFailure = this->failure;
        }

        if (currentFailure) {
            std::rethrow_exception(currentFailure);
        }
    }

    void stop() {
        this->stop_without_rethrow();
        this->rethrow_if_failed();
    }

private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::uint32_t MaximumCatchUpTicks = 4;

    static Clock::time_point advance_deadline(
        Clock::time_point deadline,
        Clock::time_point finished,
        ThreadDuration interval,
        std::uint32_t& catchUpTicks) noexcept {
        const Clock::time_point nextDeadline = deadline + interval;

        if (nextDeadline > finished) {
            catchUpTicks = 0;
            return nextDeadline;
        }

        if (++catchUpTicks >= MaximumCatchUpTicks) {
            catchUpTicks = 0;
            return finished + interval;
        }

        return nextDeadline;
    }

    static ThreadDuration elapsed_since(
        Clock::time_point previous,
        Clock::time_point current) noexcept {
        const ThreadDuration elapsed =
            std::chrono::duration_cast<ThreadDuration>(current - previous);

        return elapsed > ThreadDuration::zero()
            ? elapsed
            : ThreadDuration(1);
    }

    void run(Task& task) noexcept {
        std::exception_ptr currentFailure;
        bool started = false;

        try {
            task.start();
            started = true;
            this->run_ticks(task);
        } catch (...) {
            currentFailure = std::current_exception();
        }

        if (started) {
            try {
                task.stop();
            } catch (...) {
                if (!currentFailure) {
                    currentFailure = std::current_exception();
                }
            }
        }

        {
            const std::scoped_lock lock(this->mutex);
            this->failure = currentFailure;
            this->isRunning = false;
            this->isPaused = false;
        }

        this->wake.notify_all();
    }

    void run_ticks(Task& task) {
        Clock::time_point previousTick = Clock::now();
        Clock::time_point deadline = previousTick;
        std::uint64_t observedRevision = 0;
        std::uint32_t catchUpTicks = 0;

        while (true) {
            std::unique_lock lock(this->mutex);

            if (this->stopRequested) {
                break;
            }

            if (this->isPaused) {
                this->wake.wait(
                    lock,
                    [this] {
                        return this->stopRequested ||
                            !this->isPaused;
                    });

                if (this->stopRequested) {
                    break;
                }

                observedRevision = this->revision;
                previousTick = Clock::now();
                deadline = previousTick + this->tickInterval;
                catchUpTicks = 0;
            }

            if (observedRevision != this->revision) {
                observedRevision = this->revision;
                previousTick = Clock::now();
                deadline = previousTick + this->tickInterval;
                catchUpTicks = 0;
            }

            if (this->stopRequested) {
                break;
            }

            const ThreadDuration interval = this->tickInterval;

            if (interval == ThreadDuration::zero()) {
                lock.unlock();

                const Clock::time_point currentTick = Clock::now();
                task.tick(elapsed_since(previousTick, currentTick));
                previousTick = currentTick;
                continue;
            }

            const Clock::time_point now = Clock::now();

            if (now < deadline) {
                const bool interrupted = this->wake.wait_until(
                    lock,
                    deadline,
                    [this, observedRevision] {
                        return this->stopRequested ||
                            this->isPaused ||
                            this->revision != observedRevision;
                    });

                if (interrupted) {
                    continue;
                }
            }

            lock.unlock();
            task.tick(interval);

            deadline = advance_deadline(
                deadline,
                Clock::now(),
                interval,
                catchUpTicks);
        }
    }

    void stop_without_rethrow() noexcept {
        if (!this->worker.joinable()) {
            return;
        }

        {
            const std::scoped_lock lock(this->mutex);
            this->stopRequested = true;
        }

        this->wake.notify_all();
        this->worker.join();
    }

    mutable std::mutex mutex;
    std::condition_variable wake;
    std::exception_ptr failure;
    std::uint64_t revision = 0;
    TickRate tickRate;
    ThreadDuration tickInterval;
    bool isPaused = false;
    bool isRunning = true;
    bool stopRequested = false;
    std::thread worker;
};

}
