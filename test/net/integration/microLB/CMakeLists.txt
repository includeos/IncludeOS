cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

os_add_config(net_microLB "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(net_microLB "Configure test" service.cpp)

os_add_plugins(net_microLB autoconf vfs)
os_add_drivers(net_microLB virtionet)

os_add_stdout(net_microLB default_stdout)

# TODO: This cmake command no longer exists
#os_add_os_library(net_microLB microlb)
#os_add_os_library(net_microLB liveupdate)

os_diskbuilder(net_microLB ${CMAKE_CURRENT_SOURCE_DIR}/drive)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(server.js ${CMAKE_CURRENT_BINARY_DIR})
