cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-std=c++14 HAVE_FLAG_STD_CXX14)
if(NOT HAVE_FLAG_STD_CXX14)
 message(FATAL_ERROR "The provided compiler: " ${CMAKE_CXX_COMPILER} "\n does not support c++14 standard please make sure your CC and CXX points to a compiler that supports c++14")
endif()

set(SOURCES vmbuild.cpp)
set(ELF_SYMS_SOURCES elf_syms.cpp ../src/util/crc32.cpp)



set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O2 -g")

# TODO: write scripts that automatically find include directories
include_directories(
  .
  ${INCLUDE_PATH}/include
  ./../mod/GSL/
  ../api)

add_executable(vmbuild ${SOURCES})
target_link_libraries(vmbuild stdc++)
add_executable(elf_syms ${ELF_SYMS_SOURCES})
target_link_libraries(elf_syms stdc++)
#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS vmbuild elf_syms DESTINATION ${CMAKE_INSTALL_PREFIX})
