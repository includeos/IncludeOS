cmake_minimum_required(VERSION 2.8.9)

project (diskimagebuilder)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-std=c++14 HAVE_FLAG_STD_CXX14)
if(NOT HAVE_FLAG_STD_CXX14)
 message(FATAL_ERROR "The provided compiler: " ${CMAKE_CXX_COMPILER} "\n does not support c++14 standard please make sure your CC and CXX points to a compiler that supports c++14")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O3")

set(SOURCES main.cpp filetree.cpp writer.cpp)

include_directories(../mod/GSL)
add_executable(diskbuilder ${SOURCES})
target_link_libraries(diskbuilder stdc++)
#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS diskbuilder DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
