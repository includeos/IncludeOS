cmake_minimum_required(VERSION 2.8.12)
project(URI)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Travis
add_subdirectory(src)
enable_testing()
add_subdirectory(test)