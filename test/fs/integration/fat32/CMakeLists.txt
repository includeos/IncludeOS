cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_fat32)

set(SERVICE_NAME "FAT32 test")
set(BINARY       "test_fat32")
set(MAX_MEM 128)
set(SOURCES
    fat32.cpp
  )
#set(LOCAL_INCLUDES ".")

set(DRIVERS virtioblk)

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
