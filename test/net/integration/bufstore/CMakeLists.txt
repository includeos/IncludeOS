cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_bufstore)

set(SERVICE_NAME "Bufferstore test")
set(BINARY       "test_bufstore")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )

#set(DRIVERS virtionet)

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
