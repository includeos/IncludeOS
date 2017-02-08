cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (mender_demo)

set(SERVICE_NAME "IncludeOS Mender Example")

set(BINARY       "mender_demo")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)

set(SOURCES
  service.cpp
  )

set(LOCAL_INCLUDES
  "$ENV{INCLUDEOS_PREFIX}/includeos/include/botan-2" # Assumes botan is installed in your IncludeOS installation
  # Assumes mender and liveupdate headers are installed in your IncludeOS installation
  )

set(DRIVERS
  virtionet   # Virtio networking
  )

set(LIBRARIES
  # Assumes the libraries are installed in your IncludeOS installation
  "$ENV{INCLUDEOS_PREFIX}/includeos/lib/libmender_client.a"
  "$ENV{INCLUDEOS_PREFIX}/includeos/lib/libliveupdate.a"
  "$ENV{INCLUDEOS_PREFIX}/includeos/lib/libbotan-2.a"
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

# Build private key into disk
diskbuilder(disk disk.img)
