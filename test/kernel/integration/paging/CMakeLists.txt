cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(kernel_paging "Page protection test" ${SOURCES})
os_add_stdout(kernel_paging default_stdout)
os_add_drivers(kernel_paging boot_logger)
os_add_plugins(kernel_paging vfs)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
