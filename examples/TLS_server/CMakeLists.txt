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
project (tls_server)
include(os)

set(SOURCES
  service.cpp http.cpp
)


os_add_executable(tls_service "TLS server" ${SOURCES})

# DRIVERS / PLUGINS:

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(tls_service solo5net)
else()
  os_add_drivers(tls_service
    virtionet     # Virtio networking
    vmxnet3
  )
endif()
os_add_plugins(tls_service autoconf vfs)
os_add_stdout(tls_service default_stdout)

# Build memdisk content
os_diskbuilder(tls_service drive)
