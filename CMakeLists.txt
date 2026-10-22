cmake_minimum_required(VERSION 3.24)

project(Northrend VERSION 0.0.1 LANGUAGES C CXX)

# Select platform integrations

string(TOLOWER "${CMAKE_SYSTEM_NAME}" NORTHREND_SYSTEM_NAME)
set(NORTHREND_PLATFORM_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform/${NORTHREND_SYSTEM_NAME}.cmake")

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

add_library(northrend-compile-options INTERFACE)
add_library(Northrend::CompileOptions ALIAS northrend-compile-options)

if(MSVC)
  include(cmake/compiler/msvc/settings.cmake)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  include(cmake/compiler/clang/settings.cmake)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include(cmake/compiler/gcc/settings.cmake)
else()
  message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

include(CheckIPOSupported)

check_ipo_supported(
  RESULT IPO_SUPPORTED
  OUTPUT IPO_ERROR
  LANGUAGES CXX)

if(NOT IPO_SUPPORTED)
  message(WARNING "Interprocedural optimization is not supported: ${IPO_ERROR}")
endif()

# Build dependencies and sources

include(cmake/functions/AddGame.cmake)

add_subdirectory(dep)
add_subdirectory(src)
