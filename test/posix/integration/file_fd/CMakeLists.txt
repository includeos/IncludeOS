cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project(test_file_fd)

set(SERVICE_NAME "File_fd test")
set(BINARY       "test_file_fd")
set(MAX_MEM 128)
set(SOURCES
    test_file_fd.cpp
  )
#set(LOCAL_INCLUDES ".")

set_property(SOURCE test_file_fd.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-unused-function ")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

diskbuilder(disk disk.img)
