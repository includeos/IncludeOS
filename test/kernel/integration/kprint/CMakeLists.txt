cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT /usr/local)
endif()

MESSAGE(STATUS "CMake root: " ${INCLUDEOS_ROOT})

set(SERVICE_NAME "kprint test")
set(BINARY       "test_kprint")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include(${INCLUDEOS_ROOT}/includeos/service.cmake)
