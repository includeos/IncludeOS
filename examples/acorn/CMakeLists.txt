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
include(dependencies.cmake)

set(SOURCES
  service.cpp fs/acorn_fs.cpp
)

os_add_config(acorn "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(acorn "Acorn Web Appliance" ${SOURCES})

os_include_directories(acorn PRIVATE "app/" ${BUCKET_DIR})


# Make sure bucket is downloaded before service is built
os_add_dependencies(acorn bucket)

# DRIVERS / PLUGINS:

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(acorn solo5net)
else()
  os_add_drivers(acorn
    virtionet     # Virtio networking
    vmxnet3
  )
endif()
os_add_plugins(acorn autoconf)
os_add_stdout(acorn default_stdout)

os_add_os_library(acorn mana)

# Build memdisk content
os_diskbuilder(acorn disk1)
