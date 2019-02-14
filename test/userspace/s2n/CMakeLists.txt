cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

# Human-readable name of your service
set(SERVICE_NAME "Linux userspace S2N test")
# Name of your service binary
set(BINARY       "linux_s2n")

set(INCLUDES
  )

set(SOURCES
    service.cpp
    serial.cpp
  )

option(ENABLE_S2N "" ON)

include($ENV{INCLUDEOS_PREFIX}/includeos/linux.service.cmake)
