cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (mender_demo)

set(SERVICE_NAME "IncludeOS Mender Example")
set(BINARY       "mender_demo")

set(SOURCES service.cpp)

set(DRIVERS virtionet vmxnet3)

set(LIBRARIES
  "libmender.a"
  "libliveupdate.a"
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Build private key into disk
diskbuilder(disk disk.img)
