cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
  )

os_add_executable(net_gateway "Gateway test" ${SOURCES})

os_add_nacl(net_gateway nacl.txt)
os_add_drivers(net_gateway virtionet e1000 vmxnet3)
os_add_stdout(net_gateway default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
