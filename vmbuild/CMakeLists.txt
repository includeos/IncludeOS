cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

set(SOURCES vmbuild.cpp)
set(ELF_SYMS_SOURCES elf_syms.cpp ../src/util/crc32.cpp)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O2 -g")

# TODO: write scripts that automatically find include directories
include_directories(
  .
  ${INCLUDE_PATH}/include
  ./../mod/GSL/
  ../api)

add_executable(vmbuild ${SOURCES})
add_executable(elf_syms ${ELF_SYMS_SOURCES})

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS vmbuild elf_syms DESTINATION ${CMAKE_INSTALL_PREFIX})
