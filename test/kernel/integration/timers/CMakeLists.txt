cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
)

os_add_executable(kernel_timers "Timers Test Service" service.cpp timers.cpp)
os_add_stdout(kernel_timers default_stdout)
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
