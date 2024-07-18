cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(stl_crt "C/C++ runtime test" ${SOURCES})
os_add_stdout(stl_crt default_stdout)

configure_file (test.py ${CMAKE_CURRENT_BINARY_DIR})
