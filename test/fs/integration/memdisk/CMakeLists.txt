cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(memdisk)
include(os)

set(SOURCES
    twosector.cpp
)

os_add_executable(fs_memdisk "Memdisk filesystem test" ${SOURCES})
os_add_stdout(fs_memdisk default_stdout)

os_add_memdisk(fs_memdisk ${CMAKE_CURRENT_SOURCE_DIR}/sector2.disk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
