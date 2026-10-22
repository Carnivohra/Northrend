if(TARGET flecs::flecs_static)
  return()
endif()

include(FetchContent)

FetchContent_Declare(Flecs
  URL https://github.com/SanderMertens/flecs/archive/refs/tags/v4.1.6.tar.gz
  URL_HASH SHA256=29ccf56961b7ffbd38cce2227a06c0722c7df464422e86619a65ee37bb31bae7
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR northrend-no-cmake)

FetchContent_MakeAvailable(Flecs)

# Compile the amalgamated source as a static ECS core.

add_library(flecs-static STATIC
  ${flecs_SOURCE_DIR}/distr/flecs.c)
add_library(flecs::flecs_static ALIAS flecs-static)

target_include_directories(flecs-static
  PUBLIC
    ${flecs_SOURCE_DIR}/distr)

target_compile_definitions(flecs-static
  PUBLIC
    FLECS_CPP
    FLECS_CPP_NO_ENUM_REFLECTION
    FLECS_CUSTOM_BUILD)

target_compile_features(flecs-static
  PRIVATE
    c_std_99)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(flecs-static
    PRIVATE
      -Wno-discarded-qualifiers)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(flecs-static
    PRIVATE
      -Wno-incompatible-pointer-types-discards-qualifiers)
endif()
