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
project(router_service C CXX)
include(os)
# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
)
os_add_executable(router_service "IncludeOS Router Service" ${SOURCES})

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS
      solo5net
    )
else()
  set(DRIVERS
      virtionet     # Virtio networking
      # virtioblock # Virtio block device
      # ... Others from src/drivers
    )
endif()

os_add_drivers(router_service ${DRIVERS})
