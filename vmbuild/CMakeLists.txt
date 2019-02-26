cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

include(CheckCXXCompilerFlag)
set(CMAKE_BUILD_TYPE Release)
check_cxx_compiler_flag(-std=c++14 HAVE_FLAG_STD_CXX14)
if(NOT HAVE_FLAG_STD_CXX14)
 message(FATAL_ERROR "The provided compiler: " ${CMAKE_CXX_COMPILER} "\n does not support c++14 standard please make sure your CC and CXX points to a compiler that supports c++14")
endif()

set(SOURCES vmbuild.cpp)
set(ELF_SYMS_SOURCES elf_syms.cpp ../src/util/crc32.cpp)



set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O2 -g")


if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  ##needed by conaningans

  include(${CMAKE_BINARY_DIR}/conan.cmake)
  conan_cmake_run(
    REQUIRES GSL/2.0.0@includeos/test
    BASIC_SETUP
  )
endif(CONAN_EXPORTED)
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
