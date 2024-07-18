cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
  )

os_add_executable(net_router "Routing test" ${SOURCES})

os_add_drivers(net_router virtionet)
os_add_stdout(net_router default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(setup.sh ${CMAKE_CURRENT_BINARY_DIR})
