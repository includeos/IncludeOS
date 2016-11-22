cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT /usr/local)
else()
  set(INCLUDEOS_ROOT $ENV{INCLUDEOS_ROOT})
endif()

set(SERVICE_NAME "Memdisk test")
set(BINARY       "test_memdisk")
set(MAX_MEM 64)
set(SOURCES
    nosector.cpp
  )
#set(LOCAL_INCLUDES ".")

set(MEMDISK ${CMAKE_SOURCE_DIR}/sector0.disk)

# include service build script
include(${INCLUDEOS_ROOT}/includeos/service.cmake)
