include_guard(GLOBAL)

include(FetchContent)

set(NORTHREND_JAVA_HOME
  ""
  CACHE PATH
  "Java Development Kit used by Android builds")

# Validate a JDK home without relying on the caller's environment.

function(northrend_validate_java_home candidate result)
  if(CMAKE_HOST_WIN32)
    set(java "${candidate}/bin/java.exe")
    set(javac "${candidate}/bin/javac.exe")
  else()
    set(java "${candidate}/bin/java")
    set(javac "${candidate}/bin/javac")
  endif()

  if(NOT EXISTS "${java}" OR NOT EXISTS "${javac}")
    set(${result} "" PARENT_SCOPE)
    return()
  endif()

  execute_process(
    COMMAND "${java}" -version
    OUTPUT_VARIABLE java_version_output
    ERROR_VARIABLE java_version_output
    RESULT_VARIABLE java_version_result)

  if(NOT java_version_result EQUAL 0)
    set(${result} "" PARENT_SCOPE)
    return()
  endif()

  string(REGEX MATCH
    "version \"([0-9]+)"
    version_match
    "${java_version_output}")

  if(version_match AND CMAKE_MATCH_1 GREATER_EQUAL 17)
    set(${result} "${candidate}" PARENT_SCOPE)
  else()
    set(${result} "" PARENT_SCOPE)
  endif()
endfunction()

# Select a pinned Temurin JDK for the active build host.

function(northrend_select_jdk_package url hash)
  string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" host_architecture)

  if(host_architecture MATCHES "^(amd64|x86_64|x64)$")
    set(host_architecture x64)
  elseif(host_architecture MATCHES "^(aarch64|arm64)$")
    set(host_architecture aarch64)
  else()
    message(FATAL_ERROR
      "Automatic Java setup does not support host architecture: "
      "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  endif()

  set(version "17.0.20.1_1")
  set(release "jdk-17.0.20.1%2B1")
  set(base_url
    "https://github.com/adoptium/temurin17-binaries/releases/download/${release}")

  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    if(host_architecture STREQUAL "x64")
      set(package_hash
        "3808d1d15e3ec6bd5b84057fb5d84c33d8a1536a258146bcea2e603fc726e08e")
    else()
      set(package_hash
        "457b57af8f9c93ec39080bb8c764f559dc8c89a6da1a39d718a400b7890d3e41")
    endif()

    set(package
      "OpenJDK17U-jdk_${host_architecture}_linux_hotspot_${version}.tar.gz")
  elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(NOT host_architecture STREQUAL "x64")
      message(FATAL_ERROR
        "Automatic Java setup currently supports only x64 Windows hosts")
    endif()

    set(package
      "OpenJDK17U-jdk_x64_windows_hotspot_${version}.zip")
    set(package_hash
      "e53a79c3c3d86865bd7e787903884331068e71321714ffd44f145785affc7cb0")
  elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(host_architecture STREQUAL "x64")
      set(package_hash
        "c01975da12ed4235250ff891fe8bba73a9e73037d444b269c9d0922b5dbc8e0a")
    else()
      set(package_hash
        "196d13ba5f10414bef7f6a05a9b3f00edacb18ebacef2b99485db9e2ee18f0e8")
    endif()

    set(package
      "OpenJDK17U-jdk_${host_architecture}_mac_hotspot_${version}.tar.gz")
  else()
    message(FATAL_ERROR
      "Automatic Java setup does not support host platform: "
      "${CMAKE_HOST_SYSTEM_NAME}")
  endif()

  set(${url} "${base_url}/${package}" PARENT_SCOPE)
  set(${hash} "${package_hash}" PARENT_SCOPE)
endfunction()

# Prefer an explicit JDK, then the environment and the system. Download a
# private toolchain only when none of those provide Java 17 or newer.

function(northrend_resolve_java result)
  if(NORTHREND_JAVA_HOME)
    northrend_validate_java_home(
      "${NORTHREND_JAVA_HOME}"
      java_home)

    if(NOT java_home)
      message(FATAL_ERROR
        "NORTHREND_JAVA_HOME is not a valid JDK: "
        "${NORTHREND_JAVA_HOME}")
    endif()
  elseif(DEFINED ENV{JAVA_HOME} AND NOT "$ENV{JAVA_HOME}" STREQUAL "")
    northrend_validate_java_home(
      "$ENV{JAVA_HOME}"
      java_home)

    if(NOT java_home)
      message(FATAL_ERROR
        "JAVA_HOME is not a valid JDK: $ENV{JAVA_HOME}")
    endif()
  else()
    find_package(Java 17 QUIET COMPONENTS Development)

    if(Java_FOUND)
      get_filename_component(java
        "${Java_JAVA_EXECUTABLE}"
        REALPATH)
      get_filename_component(java_bin "${java}" DIRECTORY)
      get_filename_component(system_java_home "${java_bin}/.." ABSOLUTE)
      northrend_validate_java_home(
        "${system_java_home}"
        java_home)
    endif()
  endif()

  if(NOT java_home)
    northrend_select_jdk_package(
      jdk_url
      jdk_hash)

    message(STATUS
      "Java 17 was not found; downloading a private Temurin JDK")

    FetchContent_Declare(NorthrendJdk
      URL "${jdk_url}"
      URL_HASH "SHA256=${jdk_hash}"
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      SOURCE_DIR "${CMAKE_BINARY_DIR}/tools/jdk")

    FetchContent_MakeAvailable(NorthrendJdk)

    if(CMAKE_HOST_WIN32)
      set(java_name java.exe)
    else()
      set(java_name java)
    endif()

    file(GLOB_RECURSE java_candidates
      LIST_DIRECTORIES FALSE
      "${northrendjdk_SOURCE_DIR}/*/bin/${java_name}"
      "${northrendjdk_SOURCE_DIR}/bin/${java_name}")

    if(NOT java_candidates)
      message(FATAL_ERROR
        "The downloaded JDK does not contain ${java_name}")
    endif()

    list(GET java_candidates 0 java)
    get_filename_component(java_bin "${java}" DIRECTORY)
    get_filename_component(java_home "${java_bin}/.." ABSOLUTE)

    northrend_validate_java_home(
      "${java_home}"
      validated_java_home)

    if(NOT validated_java_home)
      message(FATAL_ERROR
        "The downloaded Java package is not a complete JDK")
    endif()

    set(java_home "${validated_java_home}")
  endif()

  set(NORTHREND_JAVA_HOME
    "${java_home}"
    CACHE PATH
    "Java Development Kit used by Android builds"
    FORCE)

  message(STATUS "Android Java home: ${java_home}")
  set(${result} "${java_home}" PARENT_SCOPE)
endfunction()
