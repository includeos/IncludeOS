cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (tcp)

# Human-readable name of your service
set(SERVICE_NAME "Lua5 Example Service")

# Name of your service binary
set(BINARY       "lua5")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

set(LOCAL_INCLUDES /usr/include/x86_64-linux-gnu /usr/include/lua5.3)

set(DRIVERS
  #virtionet
  )

set(PLUGINS
  )

set(LIBRARIES /usr/lib/x86_64-linux-gnu/liblua5.3.a)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
