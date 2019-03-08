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

project (mender_demo)
include(os)


os_add_executable(mender_demo "IncludeOS Mender Example" service.cpp)
os_add_os_library(mender_demo mender)
os_add_os_library(mender_demo liveupdate)
os_add_drivers(mender_demo virtionet vmxnet3)
#include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Build private key into disk
#nod idea if this is correct
os_diskbuilder(mender_demo disk)
