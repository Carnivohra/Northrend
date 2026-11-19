#include "Debug/log.hpp"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <iostream>
#endif

namespace Northrend::Log {

void error(const char* message) noexcept {
#if defined(__ANDROID__)
    __android_log_write(ANDROID_LOG_ERROR, "Northrend", message);
#else
    std::cerr << message << '\n';
#endif
}

}
