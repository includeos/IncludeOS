cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(ARCH i686)
set(PLATFORM x86_nano)

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_kprint)

MESSAGE(STATUS "CMake root: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "Kernel modules test")
set(BINARY       "test_mods")
set(SOURCES
    service.cpp hotswap.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Build the mod2 service and install to here, to use as a loadable module
include(ExternalProject)
ExternalProject_Add(mod2
  PREFIX module
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/mod2
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/mod2/build
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/mod2/build/seed ${CMAKE_CURRENT_BINARY_DIR}/
  )

add_dependencies(service mod2)
