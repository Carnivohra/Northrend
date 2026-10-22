#pragma once

#include <concepts>
#include <exception>
#include <iostream>

namespace Northrend {

template <typename Type>
concept GameApplication =
    std::default_initializable<Type> &&
    requires(Type application) {
        { application.run() } -> std::same_as<void>;
    };

template <GameApplication ApplicationType>
int launch() {
    try {
        ApplicationType application;
        application.run();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}

}
