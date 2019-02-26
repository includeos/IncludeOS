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

os_add_executable(net_icmp6 "ICMPv6 test" ${SOURCES})

os_add_stdout(net_icmp6 default_stdout)

# DRIVERS / PLUGINS:
if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(net_icmp6
      solo5net
    )
else()
  os_add_drivers(net_icmp6
    virtionet
  )
endif()

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
