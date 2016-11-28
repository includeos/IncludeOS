cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(SERVICE_NAME "VFS test")
set(BINARY       "test_vfs")
set(MAX_MEM 128)
set(SOURCES
  service.cpp
  )

set(DRIVERS virtioblk)

set(MEMDISK memdisk.disk)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
