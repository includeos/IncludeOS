#
# IRC Server CMake
#
# Custom version string of the IRC server
set(VERSION      "v0.3 IRCd")

set(SOURCES
  channel.cpp
  client.cpp
  client_commands.cpp
  client_new.cpp
  client_send.cpp
  client_timeout.cpp
  ircd.cpp
  ircsplit.cpp
  modes.cpp
  readq.cpp
  selftest.cpp
  server.cpp
  server_commands.cpp
  test.cpp
  transform.cpp
  )

if (LIVEUPDATE)
  list(APPEND SOURCES
    restore.cpp
  )
  set(LIBRARIES libliveupdate.a)
endif()

add_library(ircd ${SOURCES})
target_compile_definitions(ircd PUBLIC IRC_SERVER_VERSION="${VERSION}")
