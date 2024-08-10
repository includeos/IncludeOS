cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(kernel_rng "R.N.Geesus test" ${SOURCES})
os_add_stdout(kernel_rng default_stdout)
#os_add_drivers(service boot_logger)
os_add_plugins(kernel_rng vfs)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
