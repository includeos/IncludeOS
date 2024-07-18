cmake_minimum_required(VERSION 3.5)

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
