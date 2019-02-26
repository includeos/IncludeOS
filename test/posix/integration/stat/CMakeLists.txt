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
project (service)
include(os)

set(SOURCES
    test_stat_ftw.cpp
    ftw_tests.cpp
    stat_tests.cpp
  )

#os_add_config(service "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(posix_stat "POSIX file descriptor test" ${SOURCES})

os_add_drivers(posix_stat boot_logger)
os_add_plugins(posix_stat vfs)
os_add_stdout(posix_stat default_stdout)

# Create memdisk from folder
os_diskbuilder(posix_stat disk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
