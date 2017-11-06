cmake_minimum_required(VERSION 2.8.9)

option(threading "" OFF)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_coro)


set(SERVICE_NAME "Coroutines test")
set(BINARY       "test_coro")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )

#set(DRIVERS virtionet)
set(CAPABS "${CAPABS} -fcoroutines-ts ")


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
