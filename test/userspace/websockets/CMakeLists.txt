cmake_minimum_required(VERSION 3.31.6)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

# Human-readable name of your service
set(SERVICE_NAME "Linux userspace WebSocket test")

# Name of your service binary
set(BINARY       "websockets")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  )

set(EXTRA_LIBS
  $ENV{INCLUDEOS_PREFIX}/includeos/x86_64/lib/libcrypto.a
  $ENV{INCLUDEOS_PREFIX}/includeos/x86_64/lib/libssl.a
  -ldl
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/linux.service.cmake)
