cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(fs_virtio_block "VirtioBLK test" ${SOURCES})
os_add_stdout(fs_virtio_block default_stdout)

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(fs_virtio_block solo5blk)
else()
  os_add_drivers(fs_virtio_block virtioblk)
endif()

#do this code in cmake instead ?
#file(COPY memdisk DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/memdisk)
file(COPY image.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY cleanup.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY service.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(test.txt ${CMAKE_CURRENT_BINARY_DIR})
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})
