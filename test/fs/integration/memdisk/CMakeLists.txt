cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(INCLUDEOS_PREFIX /usr/local)
else()
  set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
endif()

set(SERVICE_NAME "Memdisk test")
set(BINARY       "test_memdisk")
set(MAX_MEM 64)
set(SOURCES
    twosector.cpp
  )
#set(LOCAL_INCLUDES ".")

set(MEMDISK sector2.disk)

# include service build script
include(${INCLUDEOS_PREFIX}/includeos/service.cmake)
