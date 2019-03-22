cmake_minimum_required(VERSION 3.0)
#service
project(service)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

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
