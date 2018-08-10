cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_virtioblk)

set(SERVICE_NAME "VirtioBlk test")
set(BINARY       "virtioblk")
set(SOURCES
    service.cpp
  )

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS solo5blk)
else()
  set(DRIVERS virtioblk)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
