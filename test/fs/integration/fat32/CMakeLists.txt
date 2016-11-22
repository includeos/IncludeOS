cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT /usr/local)
else()
  set(INCLUDEOS_ROOT $ENV{INCLUDEOS_ROOT})
endif()

set(SERVICE_NAME "FAT32 test")
set(BINARY       "test_fat32")
set(MAX_MEM 128)
set(SOURCES
    fat32.cpp
  )
#set(LOCAL_INCLUDES ".")

option(libvirtioblk "" ON)

# include service build script
include(${INCLUDEOS_ROOT}/includeos/service.cmake)
