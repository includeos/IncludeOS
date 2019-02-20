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

option(NORMAL "" ON)
if (NORMAL)
  set(SOURCES service.cpp)
else()
  set(SOURCES main_no_params.cpp)
endif()

os_add_executable(posix_main "POSIX main test" ${SOURCES})

os_add_plugins(posix_main vfs)
os_add_stdout(posix_main default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
