# SDL3 is available; Direct3D can become the default once implemented.

set(NORTHREND_DEFAULT_BACKEND SDL3)
set(NORTHREND_DEFAULT_RENDERER "")

set(NORTHREND_SUPPORTED_BACKENDS
  SDL3)

set(NORTHREND_SUPPORTED_RENDERERS "")

set(NORTHREND_PLATFORM_HEADERS
  Backend/Window/Handle/win32.hpp)
