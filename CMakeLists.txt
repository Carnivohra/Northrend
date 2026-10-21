cmake_minimum_required(VERSION 3.24)

project(Northrend VERSION 0.0.1 LANGUAGES C CXX)

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

# Build dependencies and sources

add_subdirectory(dep)
add_subdirectory(src)
