cmake_minimum_required(VERSION 3.0)

option(threading "" OFF)

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

set(coroutines ON)

include(os)

set(SOURCES
    service.cpp
)

os_add_executable(stl_coroutines "C++ coroutines test" ${SOURCES})
os_add_stdout(stl_coroutines default_stdout)

os_compile_options(stl_coroutines PRIVATE "-fcoroutines-ts")
configure_file (test.py ${CMAKE_CURRENT_BINARY_DIR})
