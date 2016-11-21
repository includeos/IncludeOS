cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT $ENV{HOME}/IncludeOS_install)
endif()

set(SERVICE_NAME "FAT16 test")
set(BINARY       "test_fat16")
set(MAX_MEM 128)
set(SOURCES
    fat16.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include(${INCLUDEOS_ROOT}/share/includeos/service.cmake)
