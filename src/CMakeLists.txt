add_library(Northrend STATIC)
add_library(Northrend::Northrend ALIAS Northrend)

target_sources(Northrend
  PRIVATE
    App/app.cpp
    Backend/session.cpp
    Backend/session.hpp
    ECS/registry.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      App/app.hpp
      Backend/application.hpp
      Backend/backend.hpp
      Backend/Window/handle.hpp
      Backend/Window/size.hpp
      Backend/Window/window.hpp
      ECS/entity.hpp
      ECS/registry.hpp
      Engine/engine.hpp
      Engine/fps.hpp
      Render/renderer.hpp
      Render/surface.hpp)

if(WIN32)
  target_sources(Northrend
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES
        Backend/Window/Handle/win32.hpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  target_sources(Northrend
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES
        Backend/Window/Handle/uikit.hpp)
elseif(APPLE)
  target_sources(Northrend
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES
        Backend/Window/Handle/cocoa.hpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(Northrend
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES
        Backend/Window/Handle/wayland.hpp
        Backend/Window/Handle/x11.hpp)
else()
  message(FATAL_ERROR "Unsupported window platform: ${CMAKE_SYSTEM_NAME}")
endif()

if(NORTHREND_BACKEND STREQUAL "SDL3")
  target_sources(Northrend
    PRIVATE
      Backend/SDL3/context.cpp
      Backend/SDL3/context.hpp
      Backend/SDL3/sdl3.cpp
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES
        Backend/SDL3/sdl3.hpp)

  target_link_libraries(Northrend
    PRIVATE
      SDL3::SDL3)
endif()

if(NORTHREND_RENDERER STREQUAL "Vulkan")
  target_sources(Northrend
    PRIVATE
      Render/Vulkan/device.cpp
      Render/Vulkan/device.hpp
      Render/Vulkan/frame.cpp
      Render/Vulkan/frame.hpp
      Render/Vulkan/swapchain.cpp
      Render/Vulkan/swapchain.hpp
      Render/Vulkan/vulkan.cpp
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES
        Render/Vulkan/vulkan.hpp)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(Northrend
      PRIVATE
        Render/Vulkan/Platform/surface.cpp
        Render/Vulkan/Platform/surface.hpp
        Render/Vulkan/Platform/wayland.cpp
        Render/Vulkan/Platform/wayland.hpp
        Render/Vulkan/Platform/x11.cpp
        Render/Vulkan/Platform/x11.hpp)

    target_compile_definitions(Northrend
      PRIVATE
        VK_USE_PLATFORM_WAYLAND_KHR
        VK_USE_PLATFORM_XLIB_KHR)
  else()
    message(FATAL_ERROR "Vulkan is not implemented for ${CMAKE_SYSTEM_NAME}")
  endif()

  target_compile_definitions(Northrend
    PRIVATE
      $<$<CONFIG:Debug>:NORTHREND_VULKAN_VALIDATION>)

  target_link_libraries(Northrend
    PRIVATE
      volk::volk)
endif()

target_compile_features(Northrend
  PUBLIC
    cxx_std_20)

if(IPO_SUPPORTED)
  set_target_properties(Northrend PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

target_link_libraries(Northrend
  PUBLIC
    flecs::flecs_static
  PRIVATE
    Northrend::CompileOptions)
