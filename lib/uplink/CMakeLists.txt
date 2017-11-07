cmake_minimum_required(VERSION 2.8.9)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${NEWLIB_INCLUDE_DIR})
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)
include_directories(${INCLUDEOS_ROOT}/mod/GSL/)

#dependencies
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)
include_directories(${INCLUDEOS_ROOT}/mod/rapidjson/include)

set(LIBRARY_NAME "uplink")

set(SOURCES
  transport.cpp
  ws_uplink.cpp
  register_plugin.cpp
  )

add_library(${LIBRARY_NAME} STATIC ${SOURCES})

install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/${ARCH}/plugins)

add_library(uplink_log STATIC uplink_log.cpp)
install(TARGETS uplink_log DESTINATION includeos/${ARCH}/drivers)
