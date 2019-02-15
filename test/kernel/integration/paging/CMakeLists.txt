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

# Service
project (pageprot)
include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(kernel_paging "Page protection test" ${SOURCES})
os_add_stdout(kernel_paging default_stdout)
os_add_drivers(kernel_paging boot_logger)
os_add_plugins(kernel_paging vfs)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
