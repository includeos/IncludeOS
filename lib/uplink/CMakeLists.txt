cmake_minimum_required(VERSION 2.8.9)

if (${ARCH} STREQUAL "x86_64")
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${MUSL_INCLUDE_DIR})
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)
include_directories(${INCLUDEOS_ROOT}/mod/GSL/)

#dependencies
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)
include_directories(${INCLUDEOS_ROOT}/mod/rapidjson/include)
include_directories(${OPENSSL_DIR}/include)

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
add_dependencies(${LIBRARY_NAME} PrecompiledLibraries)
add_dependencies(${LIBRARY_NAME} openssl_bundle)
install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/${ARCH}/plugins)

# Uplink log driver
add_library(uplink_log STATIC uplink_log.cpp)
add_dependencies(uplink_log PrecompiledLibraries)
add_dependencies(uplink_log openssl_bundle)
install(TARGETS uplink_log DESTINATION includeos/${ARCH}/drivers)
endif()
