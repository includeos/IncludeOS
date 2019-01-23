cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (tcp)

# Human-readable name of your service
set(SERVICE_NAME "TCP Example Service")

# Name of your service binary
set(BINARY       "tcp_example")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

# To add your own include paths:
# set(LOCAL_INCLUDES ".")

# Adding memdisk (expects my.disk to exist in current dir):
# set(MEMDISK ${CMAKE_SOURCE_DIR}/my.disk)

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet   # Virtio networking
  # ... Others from IncludeOS/src/drivers
  )

set(PLUGINS
  # syslogd    # Syslog over UDP
  # ...others
  )


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
