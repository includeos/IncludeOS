cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(net_http "HTTP test" ${SOURCES})
os_add_drivers(net_http virtionet)
os_add_stdout(net_http default_stdout)
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
