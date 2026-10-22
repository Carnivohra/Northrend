# Combine the Vulkan core with the surface implementation from the platform profile.

if(NOT NORTHREND_VULKAN_PLATFORM_SOURCES)
  message(FATAL_ERROR "Vulkan is not implemented for ${CMAKE_SYSTEM_NAME}")
endif()

target_sources(Northrend
  PRIVATE
    Render/Vulkan/device.cpp
    Render/Vulkan/device.hpp
    Render/Vulkan/frame.cpp
    Render/Vulkan/frame.hpp
    Render/Vulkan/Platform/surface.hpp
    Render/Vulkan/swapchain.cpp
    Render/Vulkan/swapchain.hpp
    Render/Vulkan/vulkan.cpp
    ${NORTHREND_VULKAN_PLATFORM_SOURCES}
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      Render/Vulkan/vulkan.hpp)

target_compile_definitions(Northrend
  PRIVATE
    ${NORTHREND_VULKAN_PLATFORM_DEFINITIONS}
    $<$<CONFIG:Debug>:NORTHREND_VULKAN_VALIDATION>)

target_compile_definitions(volk
  PRIVATE
    ${NORTHREND_VULKAN_PLATFORM_DEFINITIONS})

target_link_libraries(Northrend
  PRIVATE
    volk::volk)
