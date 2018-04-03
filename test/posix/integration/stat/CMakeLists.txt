cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project(test_posix_stat)

set(SERVICE_NAME "Stat test")
set(BINARY       "test_posix_stat")

set(SOURCES
    test_stat_ftw.cpp
    ftw_tests.cpp
    stat_tests.cpp
  )

set(DRIVERS
  boot_logger
)

set(PLUGINS
  vfs
)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

diskbuilder(disk)
