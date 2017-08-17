cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

# Name of your project
project (uplink)

# Name of your IncludeOS library
set(LIBRARY_NAME "uplink") # => libseed.a

# Source files to be built into your IncludeOS library
set(SOURCES
  transport.cpp # ...add more here
  ws_uplink.cpp
  register_plugin.cpp
  )

# Necessary includes to build your library
set(LOCAL_INCLUDES
  # "include"
  )

# include library build script
include($ENV{INCLUDEOS_PREFIX}/includeos/library.cmake)


# INSTALLATION (OPTIONAL):

# If CMAKE_INSTALL_PREFIX is not set, install to source directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}") # $ENV{INCLUDEOS_PREFIX}/includeos
endif()

# Where to install library
install(TARGETS ${LIBRARY_NAME} DESTINATION $ENV{INCLUDEOS_PREFIX}/includeos/${ARCH}/plugins)

# Where to install library headers
# NOTE: There is a difference between installing a list of files and a directory
# set(LIBRARY_HEADERS "include/seed")
# install(DIRECTORY ${LIBRARY_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
