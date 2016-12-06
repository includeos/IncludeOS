cmake_minimum_required(VERSION 2.8.9)

project(service)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

# Human-readable name of your service
set(SERVICE_NAME "Tar Example Service")

# Name of your service binary
set(BINARY       "tar_example")

# Maximum memory can be hard-coded into the binary
#set(MAX_MEM 128)
set(MAX_MEM 300)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp tarfile.o
  )

#
# Service CMake options
# (uncomment to enable)
#

# MISC:

# To add your own include paths:
# set(LOCAL_INCLUDES ".")

# Adding memdisk (expects my.disk to exist in current dir):
# set(MEMDISK ${CMAKE_SOURCE_DIR}/my.disk)

# DRIVERS:

set(DRIVERS
  virtionet   # Virtio networking
  # virtioblock # Virtio block device
  # ... Others from IncludeOS/src/drivers
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

add_custom_command(
  TARGET service PRE_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/create_binary_tar.sh ${CMAKE_SOURCE_DIR}/tar_example_folder.tar)
