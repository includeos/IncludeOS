cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
  )

os_add_executable(fs_virtiopmem "VirtioPMEM integration test" ${SOURCES})
os_add_drivers(fs_virtiopmem virtiopmem)
os_add_stdout(fs_virtiopmem default_stdout)

configure_file(banana.txt ${CMAKE_CURRENT_BINARY_DIR})
configure_file(padded_image.sh ${CMAKE_CURRENT_BINARY_DIR})
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})