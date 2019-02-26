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

set(ARCH i686)
set(PLATFORM x86_nano)

project (test_kprint)
include(os)

MESSAGE(STATUS "CMake root: " ${INCLUDEOS_PREFIX})

set(SOURCES
    service.cpp
    hotswap.cpp
  )

os_add_executable(kernel_modules "Kernel modules test" ${SOURCES})

# Build the mod2 service and install to here, to use as a loadable module
include(ExternalProject)
ExternalProject_Add(mod2
  PREFIX module
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mod2
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mod2/build
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/mod2/build/seed ${CMAKE_CURRENT_BINARY_DIR}/
  )

os_add_dependencies(kernel_modules mod2)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(mod1.json ${CMAKE_CURRENT_BINARY_DIR})
configure_file(mod3.json ${CMAKE_CURRENT_BINARY_DIR})
