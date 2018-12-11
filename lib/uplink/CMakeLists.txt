cmake_minimum_required(VERSION 2.8.9)

if (${ARCH} STREQUAL "x86_64")
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)
include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)

#dependencies
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)
include_directories(${S2N_INCLUDE})

set(LIBRARY_NAME "uplink")

set(SOURCES
  transport.cpp
  ws_uplink.cpp
  register_plugin.cpp
  config.cpp
  uplink.cpp
  )

# Install headers
install(DIRECTORY . DESTINATION includeos/include/uplink
  FILES_MATCHING PATTERN "*.hpp"
  PATTERN "starbase" EXCLUDE
  PATTERN "build" EXCLUDE)

# Uplink library
add_library(${LIBRARY_NAME} STATIC ${SOURCES})
if (LIVEUPDATE)
  set_target_properties(${LIBRARY_NAME} PROPERTIES COMPILE_DEFINITIONS "LIVEUPDATE")
endif()
install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/${ARCH}/plugins)

# Uplink log driver
add_library(uplink_log STATIC uplink_log.cpp)
install(TARGETS uplink_log DESTINATION includeos/${ARCH}/drivers)
endif()
