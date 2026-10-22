if(TARGET SDL3::SDL3)
  return()
endif()

include(FetchContent)

# Build SDL as a static dependency.

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_DISABLE_INSTALL ON)
set(SDL_DISABLE_INSTALL_DOCS ON)

# Northrend provides its own renderer implementations.

set(SDL_GPU OFF)
set(SDL_RENDER OFF)
set(SDL_METAL OFF)
set(SDL_VULKAN OFF)

if(NORTHREND_RENDERER STREQUAL "Vulkan")
  set(SDL_VULKAN ON)
endif()

FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
  URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(SDL3)
