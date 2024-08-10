cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(fs_ide "IDE driver test" ${SOURCES})
os_add_stdout(fs_ide default_stdout)

os_add_drivers(fs_ide ide_readonly)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
