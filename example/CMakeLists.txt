cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (mender_demo)

set(SERVICE_NAME "IncludeOS Mender Example")

set(BINARY       "mender_demo")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)

set(SOURCES
  service.cpp
  )

set(LOCAL_INCLUDES
  # Assumes mender and liveupdate headers are installed in your IncludeOS installation
  )

set(DRIVERS
  virtionet   # Virtio networking
  )

set(LIBRARIES
  # Assumes the libraries are installed in your IncludeOS installation
  "libmender_client.a"
  "libliveupdate.a"
  "libbotan-2.a"
  )

include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Build private key into disk
diskbuilder(disk disk.img)
