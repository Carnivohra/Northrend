#pragma once

#include "Debug/log.hpp"

#include <concepts>
#include <exception>
#include <memory>

namespace Northrend {

template <typename Type>
concept GameApplication = std::default_initializable<Type> && requires(Type application) {
    { application.run() } -> std::same_as<void>;
};

template <GameApplication ApplicationType> int launch() {
    try {
        auto application = std::make_unique<ApplicationType>();
        application->run();
        return 0;
    } catch (const std::exception& exception) {
        Log::error(exception.what());
        return 1;
    }
}

}
