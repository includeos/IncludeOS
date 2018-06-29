cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_slaac)

MESSAGE(STATUS "IncludeOS prefix: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "IncludeOS Slaac test")
set(BINARY       "test_slaac")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
    )

# Enable virtionet driver
set(DRIVERS virtionet)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
