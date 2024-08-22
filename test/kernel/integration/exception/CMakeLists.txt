cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ELF_SYMBOLS true)

project (service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(kernel_exception "CPU exception test" ${SOURCES})
os_add_stdout(kernel_exception default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
