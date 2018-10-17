cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

# Human-readable name of your service
set(SERVICE_NAME "Linux userspace IRC daemon")

# Name of your service binary
set(BINARY       "linux_ircd")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
    ../service.cpp
    ../autoconf.cpp
    ../ircd/channel.cpp
    ../ircd/client.cpp
    ../ircd/client_commands.cpp
    ../ircd/client_new.cpp
    ../ircd/client_send.cpp
    ../ircd/client_timeout.cpp
    ../ircd/ircd.cpp
    ../ircd/ircsplit.cpp
    ../ircd/modes.cpp
    ../ircd/readq.cpp
    ../ircd/restore.cpp
    ../ircd/selftest.cpp
    ../ircd/server.cpp
    ../ircd/server_commands.cpp
    ../ircd/test.cpp
    ../ircd/transform.cpp
  )

#set(PLUGINS autoconf)

# Custom version string of the IRC server
set(VERSION      "v0.3 IRCd")
add_definitions(-DIRC_SERVER_VERSION="${VERSION}")

include($ENV{INCLUDEOS_PREFIX}/includeos/linux.service.cmake)
