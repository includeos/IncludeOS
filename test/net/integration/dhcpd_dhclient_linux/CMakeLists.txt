cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project(test_dhcp_server_dhclient)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS DHCP server test with Linux dhclient")

# Name of your service binary
set(BINARY       "test_dhcp_server_dhclient")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
    service.cpp
  )

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
