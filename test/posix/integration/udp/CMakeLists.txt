cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (test_posix_udp)

# Human-readable name of your service
set(SERVICE_NAME "POSIX UDP Test Service")

# Name of your service binary
set(BINARY       "test_posix_udp")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)


# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet   # Virtio networking
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
