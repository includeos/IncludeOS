cmake_minimum_required(VERSION 2.8.9)

project (diskimagebuilder)
set (CMAKE_CXX_STANDARD 14)

set(SOURCES main.cpp filetree.cpp writer.cpp)

include_directories(../api)
include_directories(../mod/GSL)
add_executable(diskimagebuild ${SOURCES})

#install()
