if(TARGET glm::glm)
  return()
endif()

include(FetchContent)

FetchContent_Declare(GLM
  URL https://github.com/g-truc/glm/archive/refs/tags/1.0.3.tar.gz
  URL_HASH SHA256=6775e47231a446fd086d660ecc18bcd076531cfedd912fbd66e576b118607001
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR northrend-no-cmake)

FetchContent_MakeAvailable(GLM)

add_library(glm-header-only INTERFACE)
add_library(glm::glm ALIAS glm-header-only)

target_include_directories(glm-header-only
  INTERFACE
    ${glm_SOURCE_DIR})

target_compile_definitions(glm-header-only
  INTERFACE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_INTRINSICS
    GLM_FORCE_RADIANS)
