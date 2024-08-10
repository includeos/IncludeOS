cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

os_add_executable(fs_fat16 "FAT16 filesystem test" fat16.cpp)
os_add_stdout(fs_fat16 default_stdout)

os_diskbuilder(fs_fat16 disk)

configure_file(banana.ascii ${CMAKE_CURRENT_BINARY_DIR})
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})
