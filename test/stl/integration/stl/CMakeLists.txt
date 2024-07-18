cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(stl_stl "C++ STL test" ${SOURCES})
os_add_stdout(stl_stl default_stdout)

os_add_plugins(stl_stl vfs)
configure_file (test.py ${CMAKE_CURRENT_BINARY_DIR})
