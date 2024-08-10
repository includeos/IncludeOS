cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

option(NORMAL "" ON)
if (NORMAL)
  set(SOURCES service.cpp)
else()
  set(SOURCES main_no_params.cpp)
endif()

os_add_executable(posix_main "POSIX main test" ${SOURCES})

os_add_plugins(posix_main vfs)
os_add_stdout(posix_main default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
