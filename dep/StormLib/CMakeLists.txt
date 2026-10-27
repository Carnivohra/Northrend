if(TARGET StormLib::storm)
  return()
endif()

include(FetchContent)

# Build a self-contained static MPQ reader without tests or install rules.
# StormLib uses option() for its settings, so make normal variables authoritative
# without changing global cache values used by SDL or other dependencies.

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(STORM_BUILD_TESTS OFF)
set(STORM_SKIP_INSTALL ON)
set(STORM_USE_BUNDLED_LIBRARIES ON)

FetchContent_Declare(StormLib
  URL https://github.com/ladislav-zezula/StormLib/archive/refs/tags/v9.40.tar.gz
  URL_HASH SHA256=f80b08bae168888702a4251312d6c2279f487673df611c2b5ef4f395e810c3b1
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(StormLib)
