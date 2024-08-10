cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (smp_test)
include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(kernel_smp "SMP test" ${SOURCES})
os_add_stdout(kernel_smp default_stdout)
os_add_drivers(kernel_smp boot_logger)
#os_add_plugins(service vfs)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
