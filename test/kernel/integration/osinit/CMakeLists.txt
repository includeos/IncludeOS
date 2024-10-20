cmake_minimum_required(VERSION 3.6)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(osinit "OS initialization test" ${SOURCES})
os_add_stdout(osinit default_stdout)
os_add_drivers(osinit boot_logger)
os_add_plugins(osinit field_medic)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
