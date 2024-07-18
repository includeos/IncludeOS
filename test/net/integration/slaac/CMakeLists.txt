cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (test_slaac)
include(os)

os_add_executable(net_slaac "IncludeOS Slaac test" service.cpp)

os_add_drivers(net_slaac virtionet)
os_add_stdout(net_slaac default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
