include_guard(GLOBAL)

include(FetchContent)
include("${CMAKE_CURRENT_LIST_DIR}/java.cmake")

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

# Keep downloaded wrapper files inside the generated Android project.
# Matching files are reused without network access on subsequent configures.

function(northrend_download_gradle_file source destination hash)
  if(EXISTS "${destination}")
    file(SHA256 "${destination}" existing_hash)
    if(existing_hash STREQUAL hash)
      return()
    endif()
  endif()

  get_filename_component(directory "${destination}" DIRECTORY)
  file(MAKE_DIRECTORY "${directory}")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/gradle/gradle/v8.12.0/${source}"
    "${destination}.download"
    EXPECTED_HASH "SHA256=${hash}"
    TLS_VERIFY ON
    TIMEOUT 120
    STATUS download_status)

  list(GET download_status 0 download_result)
  list(GET download_status 1 download_error)
  if(NOT download_result EQUAL 0)
    message(FATAL_ERROR "Failed to download Gradle ${source}: ${download_error}")
  endif()

  file(RENAME "${destination}.download" "${destination}")
endfunction()

function(northrend_configure_gradle_wrapper directory)
  northrend_download_gradle_file(
    gradlew
    "${directory}/gradlew"
    6c8726ff768626b917e23a551e0eb794e1280c544d5362b991f50bae37b8c5f6)
  northrend_download_gradle_file(
    gradlew.bat
    "${directory}/gradlew.bat"
    0f3ed8f03b50934cb8c48b15a470d5c20a30a5385825e48b55bcc8ea3d8f8e18)
  northrend_download_gradle_file(
    gradle/wrapper/gradle-wrapper.jar
    "${directory}/gradle/wrapper/gradle-wrapper.jar"
    2db75c40782f5e8ba1fc278a5574bab070adccb2d21ca5a6e5ed840888448046)

  if(NOT CMAKE_HOST_WIN32)
    file(CHMOD "${directory}/gradlew"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endif()
endfunction()

# Generate the reusable Android package around a concrete game target.

function(northrend_configure_android_game)
  set(options)
  set(one_value_arguments
    APP_NAME
    BUILD_TOOLS
    COMPILE_SDK
    ICON
    MIN_SDK
    NDK_VERSION
    PACKAGE
    TARGET
    TARGET_SDK
    VERSION_CODE
    VERSION_NAME)

  cmake_parse_arguments(
    ANDROID_GAME
    "${options}"
    "${one_value_arguments}"
    ""
    ${ARGN})

  foreach(argument
      APP_NAME
      PACKAGE
      TARGET
      VERSION_CODE
      VERSION_NAME)
    if(NOT ANDROID_GAME_${argument})
      message(FATAL_ERROR
        "Android packaging for ${ANDROID_GAME_TARGET} requires ${argument}")
    endif()
  endforeach()

  if(NOT ANDROID_GAME_PACKAGE MATCHES
     "^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$")
    message(FATAL_ERROR
      "Invalid Android package name: ${ANDROID_GAME_PACKAGE}")
  endif()

  if(NOT ANDROID_GAME_COMPILE_SDK)
    set(ANDROID_GAME_COMPILE_SDK 35)
  endif()

  if(NOT ANDROID_GAME_BUILD_TOOLS)
    set(ANDROID_GAME_BUILD_TOOLS "35.0.1")
  endif()

  if(NOT ANDROID_GAME_MIN_SDK)
    set(ANDROID_GAME_MIN_SDK 24)
  endif()

  if(NOT ANDROID_GAME_NDK_VERSION)
    set(ANDROID_GAME_NDK_VERSION "28.2.13676358")
  endif()

  if(NOT ANDROID_GAME_TARGET_SDK)
    set(ANDROID_GAME_TARGET_SDK 35)
  endif()

  get_filename_component(engine_source
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.."
    ABSOLUTE)

  # Fetch SDL sources without configuring native targets on the host.
  include("${engine_source}/dep/SDL3/source.cmake")
  northrend_declare_sdl3(SOURCE_SUBDIR northrend-source-only)
  FetchContent_MakeAvailable(SDL3)

  set(sdl_java "${sdl3_SOURCE_DIR}/android-project/app/src/main/java")
  if(NOT EXISTS "${sdl_java}/org/libsdl/app/SDLActivity.java")
    message(FATAL_ERROR "SDL Android Java sources are missing: ${sdl_java}")
  endif()

  set(template_dir "${engine_source}/platform/android")
  set(output_dir "${CMAKE_BINARY_DIR}/android")

  northrend_resolve_java(java_home)
  northrend_resolve_android_sdk(
    android_sdk
    sdkmanager
    sdk_package_file
    BUILD_TOOLS "${ANDROID_GAME_BUILD_TOOLS}"
    COMPILE_SDK "${ANDROID_GAME_COMPILE_SDK}"
    NDK_VERSION "${ANDROID_GAME_NDK_VERSION}")

  file(MAKE_DIRECTORY "${output_dir}")

  northrend_configure_gradle_wrapper("${output_dir}")

  file(GLOB legacy_icons
    "${output_dir}/app/src/main/res/mipmap-*/ic_launcher.png")
  file(REMOVE
    ${legacy_icons}
    "${output_dir}/app/src/main/res/drawable/app_icon.png")

  file(COPY "${template_dir}/"
    DESTINATION "${output_dir}"
    PATTERN "*.in" EXCLUDE)

  # Remove the old vendored copy from previously generated build directories.
  file(REMOVE_RECURSE "${output_dir}/app/src/main/java/org/libsdl/app")

  file(TO_CMAKE_PATH "${sdl3_SOURCE_DIR}" NORTHREND_ANDROID_SDL_SOURCE_DIR)
  file(TO_CMAKE_PATH "${sdl_java}" NORTHREND_ANDROID_SDL_JAVA_DIR)

  set(NORTHREND_ANDROID_APP_NAME "${ANDROID_GAME_APP_NAME}")
  set(NORTHREND_ANDROID_BUILD_TOOLS "${ANDROID_GAME_BUILD_TOOLS}")
  set(NORTHREND_ANDROID_CMAKE_VERSION "${CMAKE_VERSION}")
  set(NORTHREND_ANDROID_COMPILE_SDK "${ANDROID_GAME_COMPILE_SDK}")
  set(NORTHREND_ANDROID_ENGINE_SOURCE_DIR "${engine_source}")
  set(NORTHREND_ANDROID_GAME_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
  set(NORTHREND_ANDROID_MIN_SDK "${ANDROID_GAME_MIN_SDK}")
  set(NORTHREND_ANDROID_NDK_VERSION "${ANDROID_GAME_NDK_VERSION}")
  set(NORTHREND_ANDROID_PACKAGE "${ANDROID_GAME_PACKAGE}")
  set(NORTHREND_ANDROID_TARGET_SDK "${ANDROID_GAME_TARGET_SDK}")
  set(NORTHREND_ANDROID_VERSION_CODE "${ANDROID_GAME_VERSION_CODE}")
  set(NORTHREND_ANDROID_VERSION_NAME "${ANDROID_GAME_VERSION_NAME}")

  if(ANDROID_GAME_ICON)
    get_filename_component(icon
      "${ANDROID_GAME_ICON}"
      ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    if(NOT EXISTS "${icon}")
      message(FATAL_ERROR "Android icon does not exist: ${icon}")
    endif()

    file(MAKE_DIRECTORY "${output_dir}/app/src/main/res/drawable")
    file(COPY_FILE
      "${icon}"
      "${output_dir}/app/src/main/res/drawable/app_icon.png"
      ONLY_IF_DIFFERENT)
    set(NORTHREND_ANDROID_ICON_ATTRIBUTE
      "        android:icon=\"@drawable/app_icon\"")
  else()
    set(NORTHREND_ANDROID_ICON_ATTRIBUTE)
  endif()

  configure_file(
    "${template_dir}/settings.gradle.in"
    "${output_dir}/settings.gradle"
    @ONLY)

  file(TO_CMAKE_PATH "${android_sdk}" android_sdk_property)
  string(REPLACE ":" "\\:" android_sdk_property "${android_sdk_property}")
  file(WRITE "${output_dir}/local.properties"
    "sdk.dir=${android_sdk_property}\n")
  configure_file(
    "${template_dir}/app/build.gradle.in"
    "${output_dir}/app/build.gradle"
    @ONLY)
  configure_file(
    "${template_dir}/app/src/main/AndroidManifest.xml.in"
    "${output_dir}/app/src/main/AndroidManifest.xml"
    @ONLY)
  configure_file(
    "${template_dir}/app/src/main/res/values/strings.xml.in"
    "${output_dir}/app/src/main/res/values/strings.xml"
    @ONLY)

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(gradle_task assembleRelease)
    set(apk_variant release)
    set(apk_name app-release-unsigned.apk)
  else()
    set(gradle_task assembleDebug)
    set(apk_variant debug)
    set(apk_name app-debug.apk)
  endif()

  if(CMAKE_HOST_WIN32)
    set(gradle
      cmd
      /c
      "${output_dir}/gradlew.bat")
  else()
    set(gradle "${output_dir}/gradlew")
  endif()

  set(apk
    "${output_dir}/app/build/outputs/apk/${apk_variant}/${apk_name}")
  set(result "${CMAKE_BINARY_DIR}/${ANDROID_GAME_TARGET}.apk")

  set(install_sdk)
  if(sdk_package_file)
    set(install_sdk
      COMMAND
        "${CMAKE_COMMAND}" -E env
        "JAVA_HOME=${java_home}"
        ${sdkmanager}
        "--sdk_root=${android_sdk}"
        "--package_file=${sdk_package_file}")
  endif()

  add_custom_target(${ANDROID_GAME_TARGET}
    ${install_sdk}
    COMMAND
      "${CMAKE_COMMAND}" -E env
      "JAVA_HOME=${java_home}"
      "ANDROID_HOME=${android_sdk}"
      "ANDROID_SDK_ROOT=${android_sdk}"
      ${gradle} --project-dir "${output_dir}" "${gradle_task}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${apk}" "${result}"
    COMMENT "Building ${ANDROID_GAME_TARGET}.apk"
    USES_TERMINAL
    VERBATIM)

  message(STATUS
    "Android package output: ${result}")
endfunction()
