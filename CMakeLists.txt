cmake_minimum_required(VERSION 2.8.9)
# default IncludeOS location #FIXME#
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project(ircd)

# Human-readable name of your service
set(SERVICE_NAME "IRC service")
# Name of your service binary
set(BINARY       "IRCd")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  channel.cpp     client.cpp      client_new.cpp   modes.cpp    test.cpp     transform.cpp
  client_commands.cpp             client_send.cpp  ircd.cpp     ircsplit.cpp readq.cpp
  restore.cpp     service.cpp     server.cpp       server_commands.cpp       timeout.cpp
  )

# To add your own include paths:
set(LOCAL_INCLUDES "include")

# DRIVERS / PLUGINS:
set(DRIVERS
  virtionet
  #vmxnet3
  silent_start
  #heap_debugging
  )

set(LIBRARIES ../../LiveUpdate/build/libliveupdate.a)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Uncomment this to build vanilla:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
