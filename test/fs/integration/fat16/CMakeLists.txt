cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(INCLUDEOS_PREFIX /usr/local)
else()
  set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
endif()

set(SERVICE_NAME "FAT16 test")
set(BINARY       "test_fat16")
set(MAX_MEM 128)
set(SOURCES
    fat16.cpp
  )

set(MEMDISK my.disk)

# include service build script
include(${INCLUDEOS_PREFIX}/includeos/service.cmake)
