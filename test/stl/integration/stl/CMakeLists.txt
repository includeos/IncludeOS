cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project(service)

set(SERVICE_NAME "STL test")
set(BINARY       "test_STL")
set(SOURCES
    service.cpp
  )

set(PLUGINS vfs)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
