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

set(SOURCES
    service.cpp
  )

os_add_config(net_vlan "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(net_vlan "VLAN test" ${SOURCES})

os_add_plugins(net_vlan autoconf)
os_add_drivers(net_vlan virtionet e1000 vmxnet3)
os_add_stdout(net_vlan default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
