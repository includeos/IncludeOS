cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (test_memmap)

set(SERVICE_NAME "Memmap test")
set(BINARY       "test_memmap")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
