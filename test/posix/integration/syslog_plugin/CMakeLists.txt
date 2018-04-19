cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_syslog_plugin)

# Human-readable name of your service
set(SERVICE_NAME "Syslog Plugin Test Service")

# Name of your service binary
set(BINARY       "test_syslog_plugin")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)


# Source files to be linked with OS library parts to form bootable image
set(SOURCES
    service.cpp
    )

set(PLUGINS syslogd syslog)
set(DRIVERS virtionet)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
