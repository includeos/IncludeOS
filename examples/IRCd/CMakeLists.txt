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
  service.cpp
  autoconf.cpp
  ircd/channel.cpp
  ircd/client.cpp
  ircd/client_commands.cpp
  ircd/client_new.cpp
  ircd/client_send.cpp
  ircd/client_timeout.cpp
  ircd/ircd.cpp
  ircd/ircsplit.cpp
  ircd/modes.cpp
  ircd/readq.cpp
  ircd/restore.cpp
  ircd/selftest.cpp
  ircd/server.cpp
  ircd/server_commands.cpp
  ircd/test.cpp
  ircd/transform.cpp
  )

# To add your own include paths:
set(LOCAL_INCLUDES "")


option(REAL_HW "Run on real hardware" ON)

if (REAL_HW)
  set(DRIVERS
      boot_logger
      e1000
      vmxnet3
      vga_output
      vga_emergency
    )
else()
  set(DRIVERS
      virtionet
      vmxnet3
      vga_output
    )
  set (PLUGINS
      uplink
    )
endif()

set(LIBRARIES libliveupdate.a)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Uncomment this to build vanilla:
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
