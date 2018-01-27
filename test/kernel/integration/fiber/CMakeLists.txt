cmake_minimum_required(VERSION 2.8.9)

if (DEFINED ENV{INCLUDEOS_THREADING})
  option(threading "" ENV{INCLUDEOS_THREADING})
endif()

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
  )

if (threading)
  set(SOURCES ${SOURCES} fiber_smp.cpp)
endif()

set(DRIVERS
  boot_logger
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
