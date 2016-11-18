cmake_minimum_required(VERSION 2.8.9)

# default IncludeOS location #FIXME#
set(INCLUDEOS_ROOT $ENV{HOME}/includeos)

set(SERVICE_NAME "IRC server")
set(BINARY       "IRCd")
set(MAX_MEM 128)
set(SOURCES
    channel.cpp     client.cpp      client_new.cpp  modes.cpp    test.cpp     transform.cpp
    client_cmd.cpp  client_msg.cpp  ircd.cpp        service.cpp  timeout.cpp
  )
set(LOCAL_INCLUDES ".")

# include service build script
include(${INCLUDEOS_ROOT}/share/includeos/service.cmake)
