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

set(DRIVERS virtioblk solo5blk)

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
