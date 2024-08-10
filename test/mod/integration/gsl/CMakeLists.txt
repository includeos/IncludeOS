cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(mod_gsl "GSL test" ${SOURCES})
os_add_stdout(mod_gsl default_stdout)

configure_file (test.py ${CMAKE_CURRENT_BINARY_DIR})
