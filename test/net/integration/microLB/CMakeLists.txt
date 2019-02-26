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

#service
project (service)
include(os)

os_add_config(net_microLB "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(net_microLB "Configure test" service.cpp)

os_add_plugins(net_microLB autoconf vfs)
os_add_drivers(net_microLB virtionet)

os_add_stdout(net_microLB default_stdout)

os_add_os_library(net_microLB microlb)
os_add_os_library(net_microLB liveupdate)

os_diskbuilder(net_microLB ${CMAKE_CURRENT_SOURCE_DIR}/drive)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(server.js ${CMAKE_CURRENT_BINARY_DIR})
