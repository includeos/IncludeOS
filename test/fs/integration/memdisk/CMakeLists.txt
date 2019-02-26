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

#service
project(service)
include(os)

set(SOURCES
    twosector.cpp
)

os_add_executable(fs_memdisk "Memdisk filesystem test" ${SOURCES})
os_add_stdout(fs_memdisk default_stdout)

os_add_memdisk(fs_memdisk ${CMAKE_CURRENT_SOURCE_DIR}/sector2.disk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
