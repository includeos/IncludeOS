cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(posix_syslog_plugin "POSIX syslog plugin test" ${SOURCES})

os_add_drivers(posix_syslog_plugin virtionet)
os_add_plugins(posix_syslog_plugin syslog syslogd)
os_add_stdout(posix_syslog_plugin default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
