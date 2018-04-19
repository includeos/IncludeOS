cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project(test_file_fd)

set(SERVICE_NAME "File_fd test")
set(BINARY       "test_file_fd")

set(SOURCES
    test_file_fd.cpp
  )

set(PLUGINS
  vfs
  )

set_property(SOURCE test_file_fd.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-unused-function ")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

diskbuilder(disk disk.img)
