cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (demo_service)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS TLS example")
# Name of your service binary
set(BINARY       "tls_client")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  http.cpp
  ../acorn/fs/acorn_fs.cpp
  )

# DRIVERS / PLUGINS:
set(DRIVERS
    virtionet     # Virtio networking
    # virtioblock # Virtio block device
    # ... Others from src/drivers
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

diskbuilder(disk)
