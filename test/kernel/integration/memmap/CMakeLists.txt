cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(kernel_memmap "Memmap test" ${SOURCES})
os_add_stdout(kernel_memmap default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm1.json ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm2.json ${CMAKE_CURRENT_BINARY_DIR})
