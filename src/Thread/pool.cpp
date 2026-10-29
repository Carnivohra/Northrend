#include "Thread/pool.hpp"

#include "Thread/worker.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Northrend {

ThreadPool::ThreadPool(std::size_t workerCount) : workerCount(workerCount) {
    if (workerCount == 0) {
        throw std::invalid_argument("Thread pool requires at least one worker");
    }

    this->workers.reserve(workerCount);

    try {
        for (std::size_t index = 0; index < workerCount; ++index) {
            this->workers.push_back(std::make_unique<Worker>([this](Worker& worker) { this->run(worker); }));
        }
    } catch (...) {
        this->stop();
        throw;
    }
}

ThreadPool::~ThreadPool() {
    this->stop();
}

std::shared_ptr<Work> ThreadPool::submit(WorkPriority priority, Work::Function function) {
    const std::size_t index = priority_index(priority);

    if (!function) {
        throw std::invalid_argument("Thread pool work must contain a function");
    }

    auto work = std::shared_ptr<Work>(new Work(priority, std::move(function)));

    {
        const std::scoped_lock lock(this->mutex);

        if (this->stopping) {
            throw std::runtime_error("Thread pool is stopped");
        }

        this->queues[index].push_back(work);
    }

    this->wake.notify_one();
    return work;
}

bool ThreadPool::promote(const std::shared_ptr<Work>& work, WorkPriority priority) {
    if (!work) {
        return false;
    }

    const std::size_t targetIndex = priority_index(priority);
    const std::scoped_lock lock(this->mutex);

    if (this->stopping || !work->queued() || priority <= work->priority) {
        return false;
    }

    auto& source = this->queues[priority_index(work->priority)];
    const auto position = std::ranges::find(source, work);

    if (position == source.end()) {
        return false;
    }

    this->queues[targetIndex].push_back(work);
    source.erase(position);
    work->priority = priority;
    this->wake.notify_one();
    return true;
}

std::size_t ThreadPool::worker_count() const noexcept {
    return this->workerCount;
}

void ThreadPool::stop() noexcept {
    {
        const std::scoped_lock lock(this->mutex);

        if (this->stopping) {
            return;
        }

        this->stopping = true;

        for (const std::unique_ptr<Worker>& worker : this->workers) {
            worker->request_stop();
        }
    }

    this->wake.notify_all();

    while (true) {
        std::shared_ptr<Work> work;

        {
            const std::scoped_lock lock(this->mutex);
            work = this->next_work();
        }

        if (!work) {
            break;
        }

        work->cancel();
    }

    for (const std::unique_ptr<Worker>& worker : this->workers) {
        worker->join();
    }

    this->workers.clear();
}

std::size_t ThreadPool::priority_index(WorkPriority priority) {
    const std::size_t index = static_cast<std::size_t>(priority);

    if (index >= PriorityCount) {
        throw std::invalid_argument("Invalid work priority");
    }

    return index;
}

bool ThreadPool::has_work() const noexcept {
    return std::ranges::any_of(this->queues, [](const auto& queue) { return !queue.empty(); });
}

std::shared_ptr<Work> ThreadPool::next_work() {
    for (std::size_t index = PriorityCount; index > 0; --index) {
        auto& queue = this->queues[index - 1];

        if (!queue.empty()) {
            std::shared_ptr<Work> work = std::move(queue.front());
            queue.pop_front();
            return work;
        }
    }

    return nullptr;
}

void ThreadPool::run(Worker& worker) {
    while (!worker.stopping()) {
        std::shared_ptr<Work> work;

        {
            std::unique_lock lock(this->mutex);
            this->wake.wait(lock, [this, &worker] { return this->stopping || worker.stopping() || this->has_work(); });

            if (this->stopping || worker.stopping()) {
                return;
            }

            work = this->next_work();
        }

        if (worker.stopping()) {
            if (work) {
                work->cancel();
            }

            return;
        }

        if (work) {
            work->execute();
        }
    }
}

}
