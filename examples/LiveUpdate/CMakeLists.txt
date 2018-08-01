cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project(service)

# Human-readable name of your service
set(SERVICE_NAME "LiveUpdate example")

# Name of your service binary
set(BINARY       "liveupdate")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp
  ../IRCd/autoconf.cpp
  ../IRCd/ircd/channel.cpp
  ../IRCd/ircd/client.cpp
  ../IRCd/ircd/client_commands.cpp
  ../IRCd/ircd/client_new.cpp
  ../IRCd/ircd/client_send.cpp
  ../IRCd/ircd/client_timeout.cpp
  ../IRCd/ircd/ircd.cpp
  ../IRCd/ircd/ircsplit.cpp
  ../IRCd/ircd/modes.cpp
  ../IRCd/ircd/readq.cpp
  ../IRCd/ircd/restore.cpp
  ../IRCd/ircd/selftest.cpp
  ../IRCd/ircd/server.cpp
  ../IRCd/ircd/server_commands.cpp
  ../IRCd/ircd/test.cpp
  ../IRCd/ircd/transform.cpp
  )

# DRIVERS / PLUGINS:
set(DRIVERS
    virtionet
    vmxnet3
    #boot_logger
  )

set(PLUGINS
    autoconf
    terminal_liu
  )

set(LIBRARIES
    libliveupdate.a
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

diskbuilder(drive)
