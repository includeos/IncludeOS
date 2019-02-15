cmake_minimum_required(VERSION 3.0)
if (DEFINED ENV{INCLUDEOS_THREADING})
  option(threading "" ENV{INCLUDEOS_THREADING})
endif()

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
    service.cpp
)
if (threading)
  list(APPEND SOURCES fiber_smp.cpp)
endif()

os_add_executable(kernel_fiber "GRUB boot test" ${SOURCES})
os_add_stdout(kernel_fiber default_stdout)
os_add_drivers(kernel_fiber boot_logger)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
