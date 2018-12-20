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

set(ARCH x86_64)

# Name of your project
project (seed)
include(os)

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

# DRIVERS / PLUGINS:
os_add_executable(seed "IncludeOS Seed" service.cpp)

#os_add_plugins(seed <plugin_name>)
os_add_stdout(seed default_stdout)
os_add_drivers(seed virtionet)
#os_add_os_library(seed <os_lib>)

#
# Service CMake options
# (uncomment to enable)
#

# MISC:

# To add your own include paths:
# os_include_directories(LOCAL_INCLUDES ".")

# Adding memdisk (expects my.disk to exist in current dir):
# os_add_memdisk(seed  ${CMAKE_SOURCE_DIR}/my.disk)

# THIRD PARTY LIBRARIES:
#os_add_library(seed <libname>)
