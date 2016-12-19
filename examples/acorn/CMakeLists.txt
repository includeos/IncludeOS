cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project (acorn)

# Download dependency
include(ExternalProject)
ExternalProject_Add(bucket
  GIT_REPOSITORY "https://github.com/includeos/bucket.git"
  PREFIX bucket
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  )

set(BUCKET_DIR ${CMAKE_CURRENT_BINARY_DIR}/bucket/src/)

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
  $ENV{INCLUDEOS_PREFIX}/includeos/include # Temp
  ${BUCKET_DIR}
  )

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet   # Virtio networking
  )

set(LIBRARIES
  $ENV{INCLUDEOS_PREFIX}/includeos/lib/libmana.a # Temp
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

# Build memdisk content
diskbuilder(disk1 disk1.img)

