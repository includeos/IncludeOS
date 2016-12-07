cmake_minimum_required(VERSION 2.8.12)
add_library(Uri uri.cpp percent_encoding.cpp)
target_include_directories(Uri PUBLIC ${CMAKE_SOURCE_DIR}/include)
