cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

# Human-readable name of your service
set(SERVICE_NAME "Micro Load Balancer")

# Name of your service binary
set(BINARY       "microlb")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  )

# DRIVERS / PLUGINS:
set(DRIVERS
  virtionet
  vmxnet3
  #vga_output
  )

set(PLUGINS
  uplink
  )

# STATIC LIBRARIES:
set(LIBRARIES
  libliveupdate.a
  libmicrolb.a
  )


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
