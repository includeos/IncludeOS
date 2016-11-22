cmake_minimum_required(VERSION 2.8.9)

set(INCLUDEOS_ROOT $ENV{INCLUDEOS_ROOT})
if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT /usr/local)
endif()

set(SERVICE_NAME "FAT16 test")
set(BINARY       "test_fat16")
set(MAX_MEM 128)
set(SOURCES
    fat16.cpp
  )
#set(LOCAL_INCLUDES ".")

set(MEMDISK ${CMAKE_SOURCE_DIR}/my.disk)

# include service build script
include(${INCLUDEOS_ROOT}/includeos/service.cmake)
