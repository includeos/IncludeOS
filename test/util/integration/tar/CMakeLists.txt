cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)

project(tar_service)

# Human-readable name of your service
set(SERVICE_NAME "Tar Test Service")

# Name of your service binary
set(BINARY       "test_tar")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  )

set(LIBRARIES ${CMAKE_SOURCE_DIR}/libtinf.a)

set(CREATE_TAR ${CMAKE_SOURCE_DIR}/tar_example)
set(TARFILE tar_example.tar)

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

# For now in service.cmake:
#add_custom_command(
  #OUTPUT tar_example.tar
  ##OUTPUT tarfile.o
  #COMMAND tar cf ${CMAKE_SOURCE_DIR}/tar_example.tar ${CMAKE_SOURCE_DIR}/tar_example
  ##COMMAND cp ${TAR_RELPATH} input.bin
  ##COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-i386 -B i386 input.bin tarfile.o
  ##COMMAND rm input.bin
#)
