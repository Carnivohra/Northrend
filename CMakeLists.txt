cmake_minimum_required(VERSION 3.24)

project(Northrend VERSION 0.0.1 LANGUAGES C CXX)

# Select platform integrations

# Platform profiles establish defaults before optional dependencies are loaded.

set(NORTHREND_PLATFORM
  "${CMAKE_SYSTEM_NAME}"
  CACHE STRING
  "Target platform built by Northrend")

set_property(CACHE NORTHREND_PLATFORM PROPERTY
  STRINGS Android Darwin iOS Linux Windows)

string(TOLOWER "${NORTHREND_PLATFORM}" NORTHREND_PLATFORM_NAME)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" NORTHREND_SYSTEM_NAME)

# A host configure creates the Android package. Gradle starts the real Android
# configure afterwards with the NDK toolchain.

set(android_bootstrap FALSE)

if(NORTHREND_PLATFORM_NAME STREQUAL "android" AND
   NOT NORTHREND_SYSTEM_NAME STREQUAL "android")
  set(android_bootstrap TRUE)
endif()

set(NORTHREND_ANDROID_BOOTSTRAP
  ${android_bootstrap}
  CACHE INTERNAL
  "Whether CMake is generating an Android package"
  FORCE)

if(NORTHREND_ANDROID_BOOTSTRAP)
  include(cmake/functions/game.cmake)
  return()
endif()

if(NOT NORTHREND_PLATFORM_NAME STREQUAL NORTHREND_SYSTEM_NAME)
  message(FATAL_ERROR
    "Northrend platform ${NORTHREND_PLATFORM} requires a "
    "${NORTHREND_PLATFORM} toolchain")
endif()

set(NORTHREND_PLATFORM_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform/${NORTHREND_PLATFORM_NAME}.cmake")

if(NOT EXISTS "${NORTHREND_PLATFORM_FILE}")
  message(FATAL_ERROR "Unsupported Northrend platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(NORTHREND_DEFAULT_BACKEND "")
set(NORTHREND_DEFAULT_RENDERER "")
set(NORTHREND_SUPPORTED_BACKENDS "")
set(NORTHREND_SUPPORTED_RENDERERS "")
set(NORTHREND_PLATFORM_HEADERS "")
set(NORTHREND_VULKAN_PLATFORM_SOURCES "")
set(NORTHREND_VULKAN_PLATFORM_DEFINITIONS "")

include("${NORTHREND_PLATFORM_FILE}")

# Consumers may override these defaults with another supported component.

set(NORTHREND_BACKEND
  "${NORTHREND_DEFAULT_BACKEND}"
  CACHE STRING
  "Window and platform backend")

set_property(CACHE NORTHREND_BACKEND PROPERTY
  STRINGS ${NORTHREND_SUPPORTED_BACKENDS})

set(NORTHREND_RENDERER
  "${NORTHREND_DEFAULT_RENDERER}"
  CACHE STRING
  "Graphics renderer")

set_property(CACHE NORTHREND_RENDERER PROPERTY
  STRINGS ${NORTHREND_SUPPORTED_RENDERERS})

if(NORTHREND_BACKEND AND
   NOT NORTHREND_BACKEND IN_LIST NORTHREND_SUPPORTED_BACKENDS)
  message(FATAL_ERROR
    "Backend ${NORTHREND_BACKEND} is not supported on ${CMAKE_SYSTEM_NAME}")
endif()

if(NORTHREND_RENDERER AND
   NOT NORTHREND_RENDERER IN_LIST NORTHREND_SUPPORTED_RENDERERS)
  message(FATAL_ERROR
    "Renderer ${NORTHREND_RENDERER} is not supported on ${CMAKE_SYSTEM_NAME}")
endif()

# Configure compiler options

# Engine and game targets share one compiler policy through this interface.

add_library(northrend-compile-options INTERFACE)
add_library(Northrend::CompileOptions ALIAS northrend-compile-options)

if(MSVC)
  include(cmake/compiler/msvc.cmake)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  include(cmake/compiler/clang.cmake)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include(cmake/compiler/gcc.cmake)
else()
  message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

include(CheckIPOSupported)

# Cache the result so game targets created from a parent project can reuse it.

check_ipo_supported(
  RESULT NORTHREND_IPO_SUPPORTED
  OUTPUT NORTHREND_IPO_ERROR
  LANGUAGES CXX)

set(NORTHREND_IPO_SUPPORTED
  "${NORTHREND_IPO_SUPPORTED}"
  CACHE INTERNAL
  "Whether Northrend targets support interprocedural optimization"
  FORCE)

if(NOT NORTHREND_IPO_SUPPORTED)
  message(WARNING
    "Interprocedural optimization is not supported: ${NORTHREND_IPO_ERROR}")
endif()

# Build dependencies and sources

# Consumer projects receive the same target factory through FetchContent.

include(cmake/functions/game.cmake)

add_subdirectory(dep)
add_subdirectory(src)
