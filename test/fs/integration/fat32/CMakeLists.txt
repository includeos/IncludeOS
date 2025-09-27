cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

os_add_executable(fs_fat32 "FAT32 filesystem test" fat32.cpp)
os_add_stdout(fs_fat32 default_stdout)

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(fs_fat32 solo5blk)
else()
  os_add_drivers(fs_fat32 virtioblk)
endif()

#do this code in cmake instead ?
configure_file(fat32_disk.sh ${CMAKE_CURRENT_BINARY_DIR})
configure_file(banana.txt ${CMAKE_CURRENT_BINARY_DIR})
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})
