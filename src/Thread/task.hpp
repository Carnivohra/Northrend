#pragma once

#include <chrono>
#include <concepts>

namespace Northrend {

using ThreadDuration = std::chrono::nanoseconds;

template <typename Type>
concept ThreadTask =
    std::move_constructible<Type> &&
    requires(Type& task, ThreadDuration delta) {
        { task.start() } -> std::same_as<void>;
        { task.tick(delta) } -> std::same_as<void>;
        { task.stop() } -> std::same_as<void>;
    };

}
