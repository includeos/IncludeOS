cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

# Human-readable name of your service
set(SERVICE_NAME "Linux userspace TCP test")

# Name of your service binary
set(BINARY       "linux_tcp")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  )

include($ENV{INCLUDEOS_PREFIX}/cmake/linux.service.cmake)
