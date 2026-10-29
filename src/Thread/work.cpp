#include "Thread/work.hpp"

#include <utility>

namespace Northrend {

Work::Work(WorkPriority priority, Function function) : priority(priority), function(std::move(function)) {}

void Work::cancel() noexcept {
    Function cancelledFunction;

    {
        const std::scoped_lock lock(this->mutex);

        if (this->status != Status::queued) {
            return;
        }

        cancelledFunction = std::move(this->function);
        this->status = Status::cancelling;
    }

    cancelledFunction = {};

    {
        const std::scoped_lock lock(this->mutex);
        this->status = Status::cancelled;
    }

    this->wake.notify_all();
}

bool Work::ready() const noexcept {
    const std::scoped_lock lock(this->mutex);
    return this->status == Status::complete || this->status == Status::failed || this->status == Status::cancelled;
}

void Work::wait() const {
    std::unique_lock lock(this->mutex);
    this->wake.wait(lock, [this] {
        return this->status == Status::complete || this->status == Status::failed || this->status == Status::cancelled;
    });

    if (this->failure) {
        std::rethrow_exception(this->failure);
    }
}

bool Work::queued() const noexcept {
    const std::scoped_lock lock(this->mutex);
    return this->status == Status::queued;
}

void Work::execute() noexcept {
    Function currentFunction;

    {
        const std::scoped_lock lock(this->mutex);

        if (this->status != Status::queued) {
            return;
        }

        currentFunction = std::move(this->function);
        this->status = Status::running;
    }

    std::exception_ptr currentFailure;

    try {
        currentFunction();
    } catch (...) {
        currentFailure = std::current_exception();
    }

    currentFunction = {};

    {
        const std::scoped_lock lock(this->mutex);
        this->failure = currentFailure;
        this->status = currentFailure ? Status::failed : Status::complete;
    }

    this->wake.notify_all();
}

}
