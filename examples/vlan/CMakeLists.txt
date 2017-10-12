cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (vlan_example)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS VLAN example")

# Name of your service binary
set(BINARY       "vlan_example")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES service.cpp)

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS
      solo5net
    )
else()
  set(DRIVERS
      virtionet
      vmxnet3
      boot_logger
    )
endif()

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
