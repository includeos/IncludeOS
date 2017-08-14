cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS seed")

# Name of your service binary
set(BINARY       "seed")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

# To add your own include paths:
# set(LOCAL_INCLUDES ".")

# DRIVERS / PLUGINS:
set(DRIVERS
  # virtionet   # Virtio networking
  # virtioblock # Virtio block device
  # ... Others from IncludeOS/src/drivers
  )

set(PLUGINS
  # syslogd    # Syslog over UDP
  # ...others
  )

# STATIC LIBRARIES:
set(LIBRARIES
  # path to full library
  )


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Create in-memory filesystem from folder
#diskbuilder(my_folder)
