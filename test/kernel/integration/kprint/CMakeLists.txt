cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_kprint)

MESSAGE(STATUS "CMake root: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "kprint test")
set(BINARY       "test_kprint")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
