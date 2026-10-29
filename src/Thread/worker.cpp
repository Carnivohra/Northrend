#include "Thread/worker.hpp"

namespace Northrend {

Worker::~Worker() {
    this->request_stop();
    this->join();
}

void Worker::request_stop() noexcept {
    this->stopRequested.store(true, std::memory_order_release);
}

void Worker::join() noexcept {
    if (this->thread.joinable()) {
        this->thread.join();
    }
}

bool Worker::stopping() const noexcept {
    return this->stopRequested.load(std::memory_order_acquire);
}

bool Worker::joinable() const noexcept {
    return this->thread.joinable();
}

}
