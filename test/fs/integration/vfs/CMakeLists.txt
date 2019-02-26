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
project(service)
include(os)

os_add_executable(fs_vfs "VFS filesystem test" service.cpp)
os_add_stdout(fs_vfs default_stdout)
os_add_drivers(fs_vfs virtioblk)
os_diskbuilder(fs_vfs memdisk)

#do this code in cmake instead ?
file(COPY memdisk DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY virtio1 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY virtio2 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY create_disk.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})
