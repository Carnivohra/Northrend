#pragma once

#if defined(__ANDROID__)

#include "Backend/Window/Handle/android.hpp"

namespace Northrend {

using NativeWindowHandle = AndroidWindowHandle;

}

#elif defined(_WIN32)

#include "Backend/Window/Handle/win32.hpp"

namespace Northrend {

using NativeWindowHandle = Win32WindowHandle;

}

#elif defined(__APPLE__)

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE
#include "Backend/Window/Handle/uikit.hpp"
#else
#include "Backend/Window/Handle/cocoa.hpp"
#endif

namespace Northrend {

#if TARGET_OS_IPHONE
using NativeWindowHandle = UIKitWindowHandle;
#else
using NativeWindowHandle = CocoaWindowHandle;
#endif

}

#elif defined(__linux__)

#include "Backend/Window/Handle/wayland.hpp"
#include "Backend/Window/Handle/x11.hpp"

#include <variant>

namespace Northrend {

using NativeWindowHandle = std::variant<WaylandWindowHandle, X11WindowHandle>;

}

#else
#error Unsupported window platform
#endif
