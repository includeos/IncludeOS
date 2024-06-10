cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

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
