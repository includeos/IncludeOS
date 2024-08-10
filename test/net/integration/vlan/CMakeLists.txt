cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
  )

os_add_config(net_vlan "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(net_vlan "VLAN test" ${SOURCES})

os_add_plugins(net_vlan autoconf)
os_add_drivers(net_vlan virtionet e1000 vmxnet3)
os_add_stdout(net_vlan default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
