cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project(test_nat)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS Gateway test")

# Name of your service binary
set(BINARY       "test_gateway")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
    service.cpp
  )

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet
  vmxnet3
  e1000
  #boot_logger
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
