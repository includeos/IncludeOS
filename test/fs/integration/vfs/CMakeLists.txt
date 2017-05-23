cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_vfs)

set(SERVICE_NAME "VFS test")
set(BINARY       "test_vfs")
set(MAX_MEM 128)
set(SOURCES
  service.cpp
  )

set(DRIVERS virtioblk)

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

diskbuilder(memdisk)
