cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ELF_SYMBOLS true)

project (service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(kernel_stacktrace "Stacktrace test" ${SOURCES})
os_add_stdout(kernel_stacktrace default_stdout)
os_add_drivers(kernel_stacktrace boot_logger)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
