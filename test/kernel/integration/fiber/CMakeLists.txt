cmake_minimum_required(VERSION 2.8.9)

option(threading "" ON)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

include_directories($ENV{INCLUDEOS_SRC}/src/include)

project (test_fiber)

MESSAGE(STATUS "CMake root: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "Fibers test")
set(BINARY       "test_fiber")
set(MAX_MEM 128)
set(SOURCES
  service.cpp
  fiber_smp.cpp
  )

set(DRIVERS
  boot_logger
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
