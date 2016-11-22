cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT $ENV{HOME}/IncludeOS_install)
endif()

set(SERVICE_NAME "IncludeOS seed")
set(BINARY       "seed")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include(${INCLUDEOS_ROOT}/share/includeos/service.cmake)
