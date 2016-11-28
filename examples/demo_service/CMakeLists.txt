cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(INCLUDEOS_PREFIX /usr/local)
else()
  set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
endif()

# Use toolchain (if needed)
set(CMAKE_TOOLCHAIN_FILE ${INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (demo_service)

set(SERVICE_NAME "IncludeOS minimal example service")
set(BINARY       "IncludeOS_example")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )

set(DRIVERS virtionet)

# include service build script
include(${INCLUDEOS_PREFIX}/includeos/service.cmake)
