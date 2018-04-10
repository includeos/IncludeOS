cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (http_client)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS HTTP Client example")

# Name of your service binary
set(BINARY       "http_client")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  )

set(DRIVERS
    virtionet
    vmxnet3
    e1000
    boot_logger
  )

set(PLUGINS vfs)

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

diskbuilder(disk)
