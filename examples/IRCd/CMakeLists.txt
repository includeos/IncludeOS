cmake_minimum_required(VERSION 3.0)

option(REAL_HW "Run on real hardware" OFF)
option(LIVEUPDATE "Enable liveupdate" ON)

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
project (ircd)

include(os)

add_subdirectory(ircd ircd2)

os_add_executable(ircd_example "IRC service"  service.cpp autoconf.cpp)
os_add_drivers(ircd_example vmxnet3)
os_add_stdout(ircd_example default_stdout)

if (LIVEUPDATE)
  os_add_os_library(ircd_example liveupdate)
endif()
os_link_libraries(ircd_example ircd)
