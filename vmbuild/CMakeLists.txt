cmake_minimum_required(VERSION 2.8.9)

project (vmbuilder)

# TODO: make sure that CXX is exported before this script is called
set(SOURCES vmbuild.cpp)
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -O3")

include_directories(.)

add_executable(vmbuild ${SOURCES})

# TODO: move install towards cmake

install(TARGETS vmbuild DESTINATION bin)
