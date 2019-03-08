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
project(protobuf_test_service C CXX)
include(os)
# Source files to be linked with OS library parts to form bootable image
set(SOURCES service.cpp person.pb.cc)
include_directories( . )
os_add_executable(protobuf_test_service "Google's protobuf runtime library test" ${SOURCES})
os_add_os_library(protobuf_test_service protobuf)
os_add_conan_package(protobuf_test_service "protobuf/3.5.1.1@includeos/test")
