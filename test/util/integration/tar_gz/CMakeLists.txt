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

project (tar_service)
include(os)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz
  COMMENT "Creating tar file"
  COMMAND tar cfvz ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz -C ${CMAKE_CURRENT_SOURCE_DIR} tar_example
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz
)

os_add_executable(util_tar_gz "Tar.gz Test Service" service.cpp)

os_add_binary_blob(util_tar_gz ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz input.bin binary)
os_add_stdout(util_tar_gz default_stdout)
os_add_drivers(util_tar_gz virtionet)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
