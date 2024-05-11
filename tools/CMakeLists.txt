cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

# TODO: Consider removing GSL dependency (the C++ guideline support library).
# I think we only use it for gsl::span which is now standardized in std::span.
# nix provides gsl as a package, but it used to be a submodule. It's header only
# so if you want to build without nix it's easy to integrate back as submoule
# or add with e.g. find_package.

include(CheckCXXCompilerFlag)
set(CMAKE_BUILD_TYPE Release)
check_cxx_compiler_flag(-std=c++14 HAVE_FLAG_STD_CXX14)
if(NOT HAVE_FLAG_STD_CXX14)
 message(FATAL_ERROR "The provided compiler: " ${CMAKE_CXX_COMPILER} "\n does not support c++14 standard please make sure your CC and CXX points to a compiler that supports c++14")
endif()

set(SOURCES src/vmbuild.cpp)
set(ELF_SYMS_SOURCES src/elf_syms.cpp src/crc32.cpp src/elf_binary.hpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O2 -g")

add_executable(vmbuild ${SOURCES})
target_link_libraries(vmbuild stdc++)
add_executable(elf_syms ${ELF_SYMS_SOURCES})
target_link_libraries(elf_syms stdc++)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS vmbuild elf_syms DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
