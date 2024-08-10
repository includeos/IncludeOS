cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

os_add_executable(net_dhclient "IncludeOS DHCP test"  service.cpp)

os_add_plugins(net_dhclient autoconf)
os_add_drivers(net_dhclient virtionet)
os_add_stdout(net_dhclient default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
