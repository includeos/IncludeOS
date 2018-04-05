cmake_minimum_required(VERSION 2.8.9)

project (diskimagebuilder)
set (CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -O3")

set(SOURCES main.cpp filetree.cpp writer.cpp)

include_directories(../mod/GSL)
add_executable(diskbuilder ${SOURCES})

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS diskbuilder DESTINATION ${CMAKE_INSTALL_PREFIX})
