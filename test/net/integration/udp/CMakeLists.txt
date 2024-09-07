cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

os_add_executable(net_udp "UDP test" service.cpp)

os_add_drivers(net_udp virtionet ip4_reassembly boot_logger)
os_add_stdout(net_udp default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
