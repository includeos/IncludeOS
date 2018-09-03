cmake_minimum_required(VERSION 2.8.9)
#option(smp "" ON)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

# Human-readable name of your service
set(SERVICE_NAME "SMP Test")
# Name of your service binary
set(BINARY       "smp")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

set(DRIVERS
    boot_logger
  )

set(PLUGINS
  # syslogd    # Syslog over UDP
  # ...others
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
