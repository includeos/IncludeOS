cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT /usr/local)
  MESSAGE(STATUS "Setting INCLUDEOS_ROOT to" ${INCLUDEOS_ROOT} )
else()
  MESSAGE(STATUS "Fetching INCLUDEOS_ROOT from ENV")
  set(INCLUDEOS_ROOT $ENV{INCLUDEOS_ROOT})
endif()

set(SERVICE_NAME "Stat test")
set(BINARY       "test_posix_stat")
set(MAX_MEM 128)
set(SOURCES
    test_stat_ftw.cpp
    ftw_tests.cpp
    stat_tests.cpp
  )
#set(LOCAL_INCLUDES ".")

set(MEMDISK ${CMAKE_SOURCE_DIR}/disk.img)

# include service build script
include(${INCLUDEOS_ROOT}/includeos/service.cmake)
