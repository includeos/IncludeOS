cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

# Use toolchain (if needed)
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)


# Name of your project
project (starbase)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS Starbase")

# Name of your service binary
set(BINARY       "starbase")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
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

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet
  vmxnet3
  uplink_log
  boot_logger
  e1000
  timestamps
  )

set(PLUGINS
  uplink
  autoconf
  system_log
  )

set(STDOUT
  vga_output
  )

# THIRD PARTY LIBRARIES:

set(LIBRARIES
  libliveupdate.a  # path to full library
  )


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

#install_certificates(disk/certs)
#diskbuilder(disk)
