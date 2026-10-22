include_guard(GLOBAL)

include(FetchContent)

set(NORTHREND_ANDROID_SDK_ROOT
  ""
  CACHE PATH
  "Android SDK used by Android builds")

# Select the pinned Android command-line tools package for the build host.

function(northrend_select_android_tools_package url hash)
  string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" host_architecture)

  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    if(NOT host_architecture MATCHES "^(amd64|x86_64|x64)$")
      message(FATAL_ERROR
        "Android command-line tools support only x64 Linux hosts")
    endif()

    set(package "commandlinetools-linux-15859902_latest.zip")
    set(package_hash
      "4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583")
  elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(NOT host_architecture MATCHES "^(amd64|x86_64|x64)$")
      message(FATAL_ERROR
        "Android command-line tools support only x64 Windows hosts")
    endif()

    set(package "commandlinetools-win-15859902_latest.zip")
    set(package_hash
      "90ae805d20434428bffcb699c290860f19bb5f66a67e6b330067e3de801fb04a")
  elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(host_architecture MATCHES "^(aarch64|arm64)$")
      set(package "commandlinetools-mac_arm64-15859902_latest.zip")
      set(package_hash
        "835b62a26162b229b441d1f6d4680383815a270809eb33522c0d480fa5002c4e")
    elseif(host_architecture MATCHES "^(amd64|x86_64|x64)$")
      set(package "commandlinetools-mac_x86_64-15859902_latest.zip")
      set(package_hash
        "c5a6378ab5cf7e0d5701921405115befff13e9ff7417fb588389338f8bd050f3")
    else()
      message(FATAL_ERROR
        "Android command-line tools do not support this macOS host: "
        "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
  else()
    message(FATAL_ERROR
      "Automatic Android SDK setup does not support host platform: "
      "${CMAKE_HOST_SYSTEM_NAME}")
  endif()

  set(${url}
    "https://dl.google.com/android/repository/${package}"
    PARENT_SCOPE)
  set(${hash} "${package_hash}" PARENT_SCOPE)
endfunction()

# Find an existing SDK before creating a private one in the build directory.

function(northrend_find_android_sdk result)
  set(candidates)

  if(NORTHREND_ANDROID_SDK_ROOT)
    get_filename_component(
      sdk_root
      "${NORTHREND_ANDROID_SDK_ROOT}"
      ABSOLUTE)
    set(${result} "${sdk_root}" PARENT_SCOPE)
    return()
  endif()

  foreach(variable ANDROID_SDK_ROOT ANDROID_HOME)
    if(DEFINED ENV{${variable}} AND NOT "$ENV{${variable}}" STREQUAL "")
      list(APPEND candidates "$ENV{${variable}}")
    endif()
  endforeach()

  if(CMAKE_HOST_WIN32 AND DEFINED ENV{LOCALAPPDATA})
    list(APPEND candidates "$ENV{LOCALAPPDATA}/Android/Sdk")
  elseif(CMAKE_HOST_APPLE AND DEFINED ENV{HOME})
    list(APPEND candidates "$ENV{HOME}/Library/Android/sdk")
  elseif(DEFINED ENV{HOME})
    list(APPEND candidates
      "$ENV{HOME}/Android/Sdk"
      "$ENV{HOME}/android-sdk")
  endif()

  foreach(candidate IN LISTS candidates)
    if(IS_DIRECTORY "${candidate}")
      get_filename_component(candidate "${candidate}" ABSOLUTE)
      set(${result} "${candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  set(${result}
    "${CMAKE_BINARY_DIR}/tools/android-sdk"
    PARENT_SCOPE)
endfunction()

# Add the command-line tools when packages are missing from the selected SDK.

function(northrend_resolve_sdkmanager sdk_root result)
  if(CMAKE_HOST_WIN32)
    set(executable sdkmanager.bat)
  else()
    set(executable sdkmanager)
  endif()

  file(GLOB managers
    LIST_DIRECTORIES FALSE
    "${sdk_root}/cmdline-tools/*/bin/${executable}"
    "${sdk_root}/tools/bin/${executable}")

  if(NOT managers)
    northrend_select_android_tools_package(
      tools_url
      tools_hash)

    message(STATUS
      "Android SDK tools were not found; downloading command-line tools")

    FetchContent_Declare(NorthrendAndroidTools
      URL "${tools_url}"
      URL_HASH "SHA256=${tools_hash}"
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

    FetchContent_MakeAvailable(NorthrendAndroidTools)

    file(GLOB_RECURSE downloaded_managers
      LIST_DIRECTORIES FALSE
      "${northrendandroidtools_SOURCE_DIR}/*/bin/${executable}"
      "${northrendandroidtools_SOURCE_DIR}/bin/${executable}")

    if(NOT downloaded_managers)
      message(FATAL_ERROR
        "The Android command-line tools do not contain ${executable}")
    endif()

    list(GET downloaded_managers 0 downloaded_manager)
    get_filename_component(tools_bin "${downloaded_manager}" DIRECTORY)
    get_filename_component(tools_root "${tools_bin}/.." ABSOLUTE)
    set(destination "${sdk_root}/cmdline-tools/latest")

    file(MAKE_DIRECTORY "${destination}")
    file(COPY "${tools_root}/"
      DESTINATION "${destination}")

    set(managers "${destination}/bin/${executable}")
  endif()

  list(SORT managers COMPARE NATURAL ORDER DESCENDING)
  list(GET managers 0 manager)

  if(CMAKE_HOST_WIN32)
    set(command cmd /c "${manager}")
  else()
    set(command "${manager}")
  endif()

  set(${result} "${command}" PARENT_SCOPE)
endfunction()

# Resolve the SDK and return only the packages that still need installation.

function(northrend_resolve_android_sdk result_root result_manager result_package_file)
  set(one_value_arguments
    BUILD_TOOLS
    COMPILE_SDK
    NDK_VERSION)

  cmake_parse_arguments(
    SDK
    ""
    "${one_value_arguments}"
    ""
    ${ARGN})

  northrend_find_android_sdk(sdk_root)
  file(MAKE_DIRECTORY "${sdk_root}")

  set(package_lines)
  if(NOT IS_DIRECTORY "${sdk_root}/platform-tools")
    string(APPEND package_lines "platform-tools\n")
  endif()
  if(NOT IS_DIRECTORY "${sdk_root}/platforms/android-${SDK_COMPILE_SDK}")
    string(APPEND package_lines
      "platforms;android-${SDK_COMPILE_SDK}\n")
  endif()
  if(NOT IS_DIRECTORY "${sdk_root}/build-tools/${SDK_BUILD_TOOLS}")
    string(APPEND package_lines
      "build-tools;${SDK_BUILD_TOOLS}\n")
  endif()
  if(NOT IS_DIRECTORY "${sdk_root}/ndk/${SDK_NDK_VERSION}")
    string(APPEND package_lines
      "ndk;${SDK_NDK_VERSION}\n")
  endif()

  if(package_lines)
    northrend_resolve_sdkmanager(
      "${sdk_root}"
      sdkmanager)

    set(package_file
      "${CMAKE_BINARY_DIR}/android-sdk-packages.txt")
    file(WRITE "${package_file}" "${package_lines}")
  else()
    set(sdkmanager)
    set(package_file)
  endif()

  set(NORTHREND_ANDROID_SDK_ROOT
    "${sdk_root}"
    CACHE PATH
    "Android SDK used by Android builds"
    FORCE)

  message(STATUS "Android SDK root: ${sdk_root}")
  set(${result_root} "${sdk_root}" PARENT_SCOPE)
  set(${result_manager} "${sdkmanager}" PARENT_SCOPE)
  set(${result_package_file} "${package_file}" PARENT_SCOPE)
endfunction()
