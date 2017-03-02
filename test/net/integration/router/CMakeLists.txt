cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (test_udp)

set(SERVICE_NAME "Routing test service")
set(BINARY       "test_router")
set(MAX_MEM 128)
set(SOURCES service.cpp)
set(DRIVERS virtionet) #vmxnet3

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
