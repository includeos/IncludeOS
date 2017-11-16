cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (test_http)

set(SERVICE_NAME "WebSocket Test")
set(BINARY       "websocket")

set(SOURCES
  service.cpp
  )

set(DRIVERS
  virtionet   # Virtio networking
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
