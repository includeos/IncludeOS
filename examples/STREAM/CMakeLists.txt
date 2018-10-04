cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (stream)

# Human-readable name of your service
set(SERVICE_NAME "STREAM Memory Benchmark Service")
# Name of your service binary
set(BINARY       "stream_example")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp stream.cpp # ...add more here
  )

# DRIVERS / PLUGINS:
set(STDOUT
  vga_output
  vga_emergency
  )

set(DRIVERS
    timestamps
  )

set(PLUGINS
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
