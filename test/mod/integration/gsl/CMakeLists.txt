cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_kprint)

MESSAGE(STATUS "CMake root: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "gsl test")
set(BINARY       "test_GSL")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
