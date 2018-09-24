cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

option(benchmark_mode "Optimizations for benchmarking" OFF)
if (benchmark_mode)
  add_definitions(-DDRIFTING_BINARY)
endif()

# Human-readable name of your service
set(SERVICE_NAME "LiveUpdate integration test")

# Name of your service binary
set(BINARY       "service")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp test_boot.cpp
  )

# DRIVERS / PLUGINS:
set(DRIVERS
    virtionet
    boot_logger
  )

set(PLUGINS
    system_log
  )

# STATIC LIBRARIES:
set(LIBRARIES
  libliveupdate.a
  )

# disable serial output
set(default_stdout OFF)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
