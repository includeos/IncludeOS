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

os_add_executable(acorn "Acorn Web Appliance" ${SOURCES})

os_include_directories(acorn PRIVATE "app/" ${BUCKET_DIR})

# Make sure bucket is downloaded before service is built
add_dependencies(acorn bucket)

# DRIVERS / PLUGINS:

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  set(DRIVERS
    solo5net
  )
else()
  set(DRIVERS
    virtionet
    vmxnet3
  )
endif()

set(LIBRARIES
  "libmana.a"
  )

# Build memdisk content
diskbuilder(acorn disk1)
