cmake_minimum_required(VERSION 2.8.9)

project (diskimagebuilder)

set(SOURCES diskimagebuild.c)

add_executable(diskimagebuild ${SOURCES})

#install()