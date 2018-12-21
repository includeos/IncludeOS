cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

#service
project(service)
include(os)

set(SOURCES
    fat32.cpp
)

os_add_executable(service "FAT32 filesystem test" ${SOURCES})
os_add_stdout(service default_stdout)

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(service solo5blk)
else()
  os_add_drivers(service virtioblk)
endif()
