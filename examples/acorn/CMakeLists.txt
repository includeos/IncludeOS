cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (acorn)

include(dependencies.cmake)

# Human-readable name of your service
set(SERVICE_NAME "Acorn Web Appliance")

# Name of your service binary
set(BINARY       "acorn")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp fs/acorn_fs.cpp
  )

# To add your own include paths:
set(LOCAL_INCLUDES
  "app/"
  ${BUCKET_DIR}
  )

# DRIVERS / PLUGINS:

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS
    solo5net
  )
else()
  set(DRIVERS
    virtionet   # Virtio networking
  )
endif()

set(LIBRARIES
  "libmana.a"
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Build memdisk content
diskbuilder(disk1)

# Make sure bucket is downloaded before service is built
add_dependencies(service bucket)
