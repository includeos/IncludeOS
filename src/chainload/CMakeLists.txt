cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(ARCH i686)
set(PLATFORM x86_nano)

option(default_stdout "" OFF)

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (chainloader)

MESSAGE(STATUS "CMake root: " $ENV{INCLUDEOS_PREFIX})

set(SERVICE_NAME "IncludeOS chainloader")
set(BINARY       "chainloader")
set(SOURCES
    service.cpp hotswap.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BINARY} DESTINATION $ENV{INCLUDEOS_PREFIX}/includeos)
