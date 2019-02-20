cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

project(chainloader)

#TODO cleanup ARCH in os.cmake and just add set platform?
set(ARCH i686)
set(PLATFORM x86_nano)

include(os)

option(default_stdout "" OFF)
set(SOURCES
    service.cpp hotswap.cpp
  )

os_add_executable(chainloader "IncludeOS chainloader" ${SOURCES})
os_install(TARGETS chainloader DESTINATION bin)
