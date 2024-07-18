cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service C CXX)

# Human-readable name of your service
set(SERVICE_NAME "Linux userspace ICMP6 test")

# Name of your service binary
set(BINARY       "icmp6")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  ../service.cpp
  )

set(EXTRA_LIBS
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/linux.service.cmake)
