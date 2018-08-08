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

# Custom version string of the IRC server
set(VERSION      "v0.4 LiveUpdate demo")

add_definitions(-DIRC_SERVER_VERSION="${VERSION}")

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

add_custom_command(
  OUTPUT motd.h
  COMMAND xxd -i < ${CMAKE_CURRENT_SOURCE_DIR}/ircd.motd > motd.h
  DEPENDS ircd.motd
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_target(motd DEPENDS motd.h)
add_dependencies(service motd)
