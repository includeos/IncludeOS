cmake_minimum_required(VERSION 3.0)

option(REAL_HW "Run on real hardware" OFF)
option(LIVEUPDATE "Enable liveupdate" OFF)

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

add_subdirectory(ircd)

set(DRIVERS
    vmxnet3
  )

set(STDOUT
  default_stdout
)

os_add_executable(service "IRC service"  service.cpp autoconf.cpp)
os_link_libraries(service ircd)
os_add_drivers(service ${DRIVERS})
os_add_stdout(service ${STDOUT})

if (LIVEUPDATE)
  os_add_os_library(service liveupdate)
endif()

