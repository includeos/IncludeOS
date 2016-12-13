cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

# TODO: make sure that CXX is exported before this script is called
set(SOURCES vmbuild.cpp elf_binary.cpp)
set(ELF_SYMS_SOURCES elf_syms.cpp)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O3")

# TODO: write scripts that automatically find include directories
include_directories(. ./../api ./../mod/GSL/)

add_executable(vmbuild ${SOURCES})
add_executable(elf_syms ${ELF_SYMS_SOURCES})

# TODO: use prefix to install into the right directory
install(TARGETS vmbuild elf_syms DESTINATION ${CMAKE_INSTALL_PREFIX})
