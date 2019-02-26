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

option(benchmark_mode "Optimizations for benchmarking" OFF)
option(SERIAL_OUTPUT "Output information" OFF)
if (benchmark_mode)
  add_definitions(-DBENCHMARK_MODE)
endif()

# Human-readable name of your service
#set(SERVICE_NAME "LiveUpdate integration test")

# Name of your service binary
#set(BINARY       "service")



# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp test_boot.cpp
  )
os_add_executable(kernel_LiveUpdate "LiveUpdate integration test" ${SOURCES})

os_add_drivers(kernel_LiveUpdate
  virtionet
  boot_logger
)

os_add_plugins(kernel_LiveUpdate
  system_log
)

os_add_os_library(kernel_LiveUpdate liveupdate)

if (SERIAL_OUTPUT)
  os_add_stdout(kernel_LiveUpdate default_stdout)
endif()

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
