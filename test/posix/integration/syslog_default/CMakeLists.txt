cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
)

#os_add_config(service "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(posix_syslog_default "POSIX syslog test" ${SOURCES})

#os_add_drivers(service boot_logger)
os_add_plugins(posix_syslog_default syslog)
os_add_stdout(posix_syslog_default default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
