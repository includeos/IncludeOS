cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(kernel_term "Terminal test" ${SOURCES})
os_add_stdout(kernel_term default_stdout)
os_add_drivers(kernel_term virtionet)
#os_add_plugins(service vfs)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
