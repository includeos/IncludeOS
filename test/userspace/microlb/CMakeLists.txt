cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

option(PORTABLE "" OFF)
option(ENABLE_S2N "" ON)

# Human-readable name of your service
set(SERVICE_NAME "Linux userspace microLB test")
# Name of your service binary
set(BINARY       "linux_microlb")

set(INCLUDES
  )

set(SOURCES
    service.cpp
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/linux.service.cmake)
