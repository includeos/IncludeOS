cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_stresstest)

set(SERVICE_NAME "Stress test")
set(BINARY       "test_stresstest")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )

set(DRIVERS virtionet)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
