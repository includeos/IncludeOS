cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project(mender)

set(LIBRARY_NAME "mender")

set(SOURCES
  src/artifact.cpp
  src/auth_manager.cpp
  src/client.cpp
  src/keystore.cpp
  src/state.cpp
  )

# Necessary includes to build your library
set(LOCAL_INCLUDES
  "include"
  # Assumes liveupdate is installed in your IncludeOS installation
  )

# include library build script
include($ENV{INCLUDEOS_PREFIX}/includeos/library.cmake)

option(verbose ON)
if(verbose)
  target_compile_definitions(${LIBRARY_NAME} PRIVATE VERBOSE_MENDER=1)
endif(verbose)

# If CMAKE_INSTALL_PREFIX is not set, install to source directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}") # $ENV{INCLUDEOS_PREFIX}/includeos
endif()

# Where to install library
install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/${ARCH}/lib)
set(LIBRARY_HEADERS "include/mender")
install(DIRECTORY ${LIBRARY_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
