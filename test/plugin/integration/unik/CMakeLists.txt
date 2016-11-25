cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(INCLUDEOS_PREFIX /usr/local)
else()
  set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
endif()

set(SERVICE_NAME "UNIK plugin test")
set(BINARY       "test_plugin_unik")
set(MAX_MEM 128)
set(SOURCES
    service.cpp
  )

set(PLUGINS unik)
set(DRIVERS virtionet)

# include service build script
include(${INCLUDEOS_PREFIX}/includeos/service.cmake)
