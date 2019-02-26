cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

# Service
project (websocket)
include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(websocket_service "WebSocket example" ${SOURCES})

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(websocket_service
      solo5net
    )
else()
  os_add_drivers(websocket_service
    virtionet
    vmxnet3
    e1000
    boot_logger
  )
endif()

os_add_stdout(websocket_service default_stdout)
os_add_plugins(websocket_service autoconf)

os_diskbuilder(websocket_service disk)
