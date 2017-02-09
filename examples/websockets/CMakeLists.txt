cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)
project (service)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS websockets example")

# Name of your service binary
set(BINARY       "websockets")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 256)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

set(DRIVERS
  virtionet   # Virtio networking
  silent_start
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
