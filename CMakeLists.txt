cmake_minimum_required(VERSION 2.8.9)
project(ircd)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

# Human-readable name of your service
set(SERVICE_NAME "IRC service")

# Name of your service binary
set(BINARY       "IRCd")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 100)


# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  channel.cpp     client.cpp      client_new.cpp   modes.cpp    test.cpp     transform.cpp
  client_commands.cpp             client_send.cpp  ircd.cpp     ircsplit.cpp readq.cpp
  restore.cpp     service.cpp     server.cpp       server_commands.cpp       timeout.cpp
  )

#
# Service CMake options
# (uncomment to enable)
#

# MISC:

# To add your own include paths:
set(LOCAL_INCLUDES "include")

# Adding memdisk (expects my.disk to exist in current dir):
#set(MEMDISK .)

# DRIVERS / PLUGINS:

set(DRIVERS
  virtionet   # Virtio networking
  silent_start
  #heap_debugging
  )

set(PLUGINS
  # syslogd    # Syslog over UDP
  # ...others
  )


set(LIBRARIES ../../LiveUpdate/build/libliveupdate.a)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
