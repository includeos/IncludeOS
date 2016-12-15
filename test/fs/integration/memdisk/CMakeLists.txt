cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (test_memdisk)

set(SERVICE_NAME "Memdisk test")
set(BINARY       "test_memdisk")
set(MAX_MEM 64)
set(SOURCES
    twosector.cpp
  )
#set(LOCAL_INCLUDES ".")

include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

add_memdisk(sector2.disk)
