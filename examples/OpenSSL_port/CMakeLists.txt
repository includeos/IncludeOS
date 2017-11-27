cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (demo_service)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS minimal example")

# Name of your service binary
set(BINARY       "IncludeOS_example")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  openssl.cpp
  )

# To add your own include paths:
set(LOCAL_INCLUDES . include)

set(LIBRARIES
    /usr/lib/x86_64-linux-gnu/libssl.a
    /usr/lib/x86_64-linux-gnu/libcrypto.a
  )

# DRIVERS / PLUGINS:
if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS
      solo5net
    )
else()
  set(DRIVERS
      virtionet     # Virtio networking
      # virtioblock # Virtio block device
      # ... Others from src/drivers
    )
endif()

set(PLUGINS
    autoconf  # configuration from config.json
    # ...others
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
