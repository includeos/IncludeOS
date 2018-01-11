cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

# TODO: make sure that CXX is exported before this script is called
set(SOURCES vmbuild.cpp)
set(ELF_SYMS_SOURCES elf_syms.cpp)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O2 -g")

# TODO: write scripts that automatically find include directories
include_directories(. ./../mod/GSL/)
include_directories(${MUSL_INCLUDE}/../../)

add_executable(vmbuild ${SOURCES})
add_executable(elf_syms ${ELF_SYMS_SOURCES})

install(TARGETS vmbuild elf_syms DESTINATION ${CMAKE_INSTALL_PREFIX})
