cmake_minimum_required(VERSION 3.5)

project (vmbuilder)

include(CheckCXXCompilerFlag)
set(CMAKE_BUILD_TYPE Release)
check_cxx_compiler_flag(-std=c++20 HAVE_FLAG_STD_CXX20)
if(NOT HAVE_FLAG_STD_CXX20)
 message(FATAL_ERROR "The provided compiler: " ${CMAKE_CXX_COMPILER} "\n does not support c++20 standard please make sure your CC and CXX points to a compiler that supports c++20")
endif()

set(SOURCES vmbuild.cpp)
set(ELF_SYMS_SOURCES elf_syms.cpp ../src/util/crc32.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2 -g")


#TODO pull vmbuild conanfile.py inn when not building with conan to get deps
# TODO: write scripts that automatically find include directories
include_directories(
  .
  ${INCLUDE_PATH}/include
  #./../mod/GSL/
  ../api)

add_executable(vmbuild ${SOURCES})
target_link_libraries(vmbuild stdc++)
add_executable(elf_syms ${ELF_SYMS_SOURCES})
target_link_libraries(elf_syms stdc++)
#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS vmbuild elf_syms DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
