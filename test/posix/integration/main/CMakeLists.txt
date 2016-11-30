cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (test_main)

set(SERVICE_NAME "Service with no Service::start, only main")
set(BINARY       "test_main")
set(MAX_MEM 128)

option(NORMAL "" ON)


if (NORMAL)
  set(SOURCES service.cpp)
else()
  set(SOURCES main_no_params.cpp)
endif()

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
