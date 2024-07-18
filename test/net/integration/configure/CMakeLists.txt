cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
  )

os_add_config(net_configure "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(net_configure "Configure test" ${SOURCES})

os_add_plugins(net_configure autoconf)
os_add_drivers(net_configure virtionet boot_logger)
os_add_stdout(net_configure default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
