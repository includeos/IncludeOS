cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS TCP Performance")

# Name of your service binary
set(BINARY       "tcp_perf")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS
      solo5net
      #solo5blk
    )
else()
  set(DRIVERS
    virtionet
    vmxnet3
    e1000
    boot_logger
  )
endif()

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
