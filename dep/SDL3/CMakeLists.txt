if(TARGET SDL3::SDL3)
  return()
endif()

include(FetchContent)

# Android packages SDL as a shared library; desktop targets link it statically.

if(ANDROID)
  set(SDL_SHARED ON)
  set(SDL_STATIC OFF)
else()
  set(SDL_SHARED OFF)
  set(SDL_STATIC ON)
endif()
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_DISABLE_INSTALL ON)
set(SDL_DISABLE_INSTALL_DOCS ON)

# Keep SDL focused on window creation and platform events. Optional engine
# systems can enable their corresponding SDL subsystem when they are added.

set(SDL_AUDIO OFF CACHE BOOL "Enable SDL audio" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "Enable SDL camera support" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "Enable SDL dialogs" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Enable SDL haptics" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "Enable SDL HIDAPI" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "Enable SDL joystick support" FORCE)
set(SDL_POWER OFF CACHE BOOL "Enable SDL power information" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Enable SDL sensors" FORCE)
set(SDL_TRAY OFF CACHE BOOL "Enable SDL tray support" FORCE)

# Northrend provides its own renderer implementations.

set(SDL_GPU OFF)
set(SDL_RENDER OFF)
set(SDL_METAL OFF)
set(SDL_OPENGL OFF CACHE BOOL "Enable SDL OpenGL support" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "Enable SDL OpenGL ES support" FORCE)
set(SDL_VULKAN OFF)

# Desktop builds use native window-system integrations rather than SDL's
# headless or direct-display video drivers.

if(NOT ANDROID)
  set(SDL_DUMMYVIDEO OFF CACHE BOOL "Enable SDL dummy video" FORCE)
  set(SDL_KMSDRM OFF CACHE BOOL "Enable SDL KMSDRM video" FORCE)
  set(SDL_OFFSCREEN OFF CACHE BOOL "Enable SDL offscreen video" FORCE)
endif()

if(NORTHREND_RENDERER STREQUAL "Vulkan")
  set(SDL_VULKAN ON)
endif()

FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
  URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(SDL3)
