cmake_minimum_required(VERSION 2.8.9)

if (${ARCH} STREQUAL "x86_64")
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

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
install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/${ARCH}/plugins)

# Uplink log driver
add_library(uplink_log STATIC uplink_log.cpp)
install(TARGETS uplink_log DESTINATION includeos/${ARCH}/drivers)
endif()
