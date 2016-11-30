cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (test_dhcp)

MESSAGE(STATUS "IncludeOS prefix: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "IncludeOS DHCP test")
set(BINARY       "test_dhcp")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
    )

# Enable virtionet driver
set(DRIVERS virtionet)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
