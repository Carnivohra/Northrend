set(NORTHREND_DEFAULT_BACKEND SDL3)
set(NORTHREND_DEFAULT_RENDERER Vulkan)

set(NORTHREND_SUPPORTED_BACKENDS
  SDL3)

set(NORTHREND_SUPPORTED_RENDERERS
  Vulkan)

set(NORTHREND_PLATFORM_HEADERS
  Backend/Window/Handle/wayland.hpp
  Backend/Window/Handle/x11.hpp)

set(NORTHREND_VULKAN_PLATFORM_SOURCES
  Render/Vulkan/Platform/surface.cpp
  Render/Vulkan/Platform/wayland.cpp
  Render/Vulkan/Platform/wayland.hpp
  Render/Vulkan/Platform/x11.cpp
  Render/Vulkan/Platform/x11.hpp)

set(NORTHREND_VULKAN_PLATFORM_DEFINITIONS
  VK_USE_PLATFORM_WAYLAND_KHR
  VK_USE_PLATFORM_XLIB_KHR)
