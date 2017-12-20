cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (test_ide)

set(SERVICE_NAME "IDE test")
set(BINARY       "test_ide")

set(SOURCES
    service.cpp
  )

set(DRIVERS
    ide_readonly
    boot_logger
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
