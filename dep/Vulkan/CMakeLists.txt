if(TARGET volk::volk)
  return()
endif()

include(FetchContent)

# Provide Vulkan headers without requiring a system Vulkan SDK.

set(VULKAN_HEADERS_ENABLE_INSTALL OFF)
set(VULKAN_HEADERS_ENABLE_TESTS OFF)

FetchContent_Declare(VulkanHeaders
  URL https://github.com/KhronosGroup/Vulkan-Headers/archive/6802bb4733b63ed5efd3adb308a6c885ef180ea1.tar.gz
  URL_HASH SHA256=9044fbb356160e43201dc2094acb44c8a906722a63a35f48fc0f051579db9bb6
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(VulkanHeaders)

# Load Vulkan entry points dynamically at runtime.

set(VOLK_INSTALL OFF)
set(VOLK_PULL_IN_VULKAN OFF)

FetchContent_Declare(volk
  URL https://github.com/zeux/volk/archive/54fc0d7a72887cbfc5b030d26897b2b40ac53fb8.tar.gz
  URL_HASH SHA256=4944450dc3943871a28ce993eaf995309eb29e7000e091309968dba4cd321648
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(volk)

target_link_libraries(volk
  PUBLIC
    Vulkan::Headers)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(volk
    PRIVATE
      VK_USE_PLATFORM_WAYLAND_KHR
      VK_USE_PLATFORM_XLIB_KHR)
endif()
